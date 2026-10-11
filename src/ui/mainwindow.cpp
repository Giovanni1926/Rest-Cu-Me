#include "mainwindow.h"
#include "floweditorview.h"
#include "toolwindow/resttoolwindow.h"
#include "explorer/explorerpanel.h"

#include <QTabWidget>
#include <QDockWidget>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QFile>
#include <QKeySequence>
#include <QMessageBox>
#include <QSettings>
#include <QFileInfo>
#include "../flow/flowfilemanager.h"
#include "../flow/flowgraph.h"

namespace rcm {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("Rest Cu Me"));
    resize(1200, 800);

    tabs_ = new QTabWidget(this);
    tabs_->setTabsClosable(true);
    tabs_->setDocumentMode(true);
    setCentralWidget(tabs_);
    connect(tabs_, &QTabWidget::tabCloseRequested, this, [this](int index) {
        QWidget* w = tabs_->widget(index);
        tabs_->removeTab(index);
        w->deleteLater();
        saveSession();
        refreshExplorer();
    });

    toolWindow_ = new RestToolWindow(this);
    dock_ = new QDockWidget(QStringLiteral("Rest Cu Me"), this);
    dock_->setObjectName(QStringLiteral("RestToolWindow"));
    dock_->setWidget(toolWindow_);
    dock_->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    dock_->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    addDockWidget(Qt::BottomDockWidgetArea, dock_);

    explorer_ = new ExplorerPanel(this);
    explorerDock_ = new QDockWidget(QStringLiteral("Explorer"), this);
    explorerDock_->setObjectName(QStringLiteral("Explorer"));
    explorerDock_->setWidget(explorer_);
    explorerDock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    explorerDock_->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    addDockWidget(Qt::LeftDockWidgetArea, explorerDock_);

    explorer_->fileActivated = [this](const QString& path) {
        for (int i = 0; i < tabs_->count(); ++i) {
            if (auto* e = dynamic_cast<FlowEditorView*>(tabs_->widget(i))) {
                if (e->filePath() == path) {
                    tabs_->setCurrentIndex(i);
                    return;
                }
            }
        }
    };
    explorer_->nodeActivated = [this](const QString& nodeId) {
        if (FlowEditorView* e = currentEditor()) e->selectNode(nodeId);
    };
    explorer_->groupActivated = [this](const QString& groupId) {
        if (FlowEditorView* e = currentEditor()) e->selectGroup(groupId);
    };
    explorer_->addRequested = [this](const QString& path, int kind) {
        FlowEditorView* target = nullptr;
        if (!path.isEmpty()) {
            for (int i = 0; i < tabs_->count(); ++i) {
                if (auto* e = dynamic_cast<FlowEditorView*>(tabs_->widget(i))) {
                    if (e->filePath() == path) {
                        target = e;
                        break;
                    }
                }
            }
        }
        if (!target) target = currentEditor();
        if (!target) return;
        tabs_->setCurrentWidget(target);
        switch (kind) {
        case AddRest: target->addRestNode(); break;
        case AddSoap: target->addSoapNode(); break;
        case AddVariables: target->addVariableNode(); break;
        case AddGroup: target->addGroup(); break;
        }
        refreshExplorer();
    };

    QMenu* fileMenu = menuBar()->addMenu(QStringLiteral("File"));
    QAction* newAction = fileMenu->addAction(QStringLiteral("Nuovo"));
    newAction->setShortcut(QKeySequence::New);
    QAction* openAction = fileMenu->addAction(QStringLiteral("Apri"));
    openAction->setShortcut(QKeySequence::Open);
    QAction* saveAction = fileMenu->addAction(QStringLiteral("Salva"));
    saveAction->setShortcut(QKeySequence::Save);
    QAction* saveAsAction = fileMenu->addAction(QStringLiteral("Salva con nome..."));
    fileMenu->addSeparator();
    QAction* exitAction = fileMenu->addAction(QStringLiteral("Esci"));
    exitAction->setShortcut(QKeySequence::Quit);

    QMenu* runMenu = menuBar()->addMenu(QStringLiteral("Esegui"));
    runAction_ = runMenu->addAction(QStringLiteral("Esegui flusso REST"));
    runAction_->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+Return")));
    runAction_->setEnabled(false);

    QMenu* viewMenu = menuBar()->addMenu(QStringLiteral("Visualizza"));
    viewMenu->addAction(explorerDock_->toggleViewAction());
    viewMenu->addAction(dock_->toggleViewAction());

    connect(newAction, &QAction::triggered, this, &MainWindow::newFlow);
    connect(openAction, &QAction::triggered, this, &MainWindow::openFlow);
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveFlow);
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::saveFlowAs);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    connect(runAction_, &QAction::triggered, this, &MainWindow::runFlow);

    connect(tabs_, &QTabWidget::currentChanged, this, [this](int) {
        runAction_->setEnabled(currentEditor() != nullptr);
        saveSession();
        refreshExplorer();
    });

    restoreSession();
    refreshExplorer();
}

FlowEditorView* MainWindow::currentEditor() const {
    return dynamic_cast<FlowEditorView*>(tabs_->currentWidget());
}

FlowEditorView* MainWindow::openEditor(const QString& filePath) {
    FlowEditorView* editor = new FlowEditorView(filePath, this);
    editor->load();
    editor->activateToolWindow = [this]() {
        dock_->show();
        dock_->raise();
        toolWindow_->showCronologia();
    };
    editor->onStructureChanged = [this]() { refreshExplorer(); };
    editor->onSelectionChanged = [this]() { refreshExplorer(); };
    const int index = tabs_->addTab(editor, QFileInfo(filePath).fileName());
    tabs_->setCurrentIndex(index);
    runAction_->setEnabled(true);
    saveSession();
    refreshExplorer();
    return editor;
}

void MainWindow::refreshExplorer() {
    std::vector<ExplorerFile> files;
    for (int i = 0; i < tabs_->count(); ++i) {
        FlowEditorView* e = dynamic_cast<FlowEditorView*>(tabs_->widget(i));
        if (!e) continue;
        ExplorerFile file;
        file.path = e->filePath();
        file.name = e->fileName();

        const FlowGraph& graph = e->graph();
        for (const auto& group : graph.groups()) {
            ExplorerGroup g;
            g.id = group.id;
            g.name = group.name;
            for (const auto& nodeId : group.nodeIds) {
                const FlowNode* node = graph.nodeById(nodeId);
                if (!node) continue;
                ExplorerNode n;
                n.id = node->id;
                n.kind = node->kind;
                n.title = node->title;
                g.nodes.push_back(n);
            }
            file.groups.push_back(g);
        }
        for (const auto& node : graph.nodes()) {
            if (graph.groupOf(node.id)) continue;
            ExplorerNode n;
            n.id = node.id;
            n.kind = node.kind;
            n.title = node.title;
            file.looseNodes.push_back(n);
        }
        files.push_back(file);
    }

    QString currentPath;
    QString selectedNodeId;
    QString selectedGroupId;
    if (FlowEditorView* e = currentEditor()) {
        currentPath = e->filePath();
        selectedNodeId = e->selectedNodeId();
        selectedGroupId = e->selectedGroupId();
    }
    explorer_->refresh(files, currentPath, selectedNodeId, selectedGroupId);
}

void MainWindow::saveSession() {
    QStringList paths;
    for (int i = 0; i < tabs_->count(); ++i) {
        if (auto* e = dynamic_cast<FlowEditorView*>(tabs_->widget(i))) {
            if (!e->filePath().isEmpty()) paths << e->filePath();
        }
    }
    QSettings settings;
    settings.setValue(QStringLiteral("session/files"), paths);
    settings.setValue(QStringLiteral("session/current"), tabs_->currentIndex());
}

void MainWindow::restoreSession() {
    QSettings settings;
    const QStringList paths = settings.value(QStringLiteral("session/files")).toStringList();
    for (const QString& path : paths) {
        if (QFileInfo::exists(path)) openEditor(path);
    }
    const int index = settings.value(QStringLiteral("session/current"), 0).toInt();
    if (index >= 0 && index < tabs_->count()) tabs_->setCurrentIndex(index);
}

void MainWindow::newFlow() {
    const QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("Nuovo flusso"), QString(), QStringLiteral("RCM Flow (*.rcmflow)"));
    if (path.isEmpty()) return;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;
    file.write(FlowFileManager::toJson(FlowGraph()).toUtf8());
    file.close();
    openEditor(path);
}

void MainWindow::openFlow() {
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("Apri flusso"), QString(), QStringLiteral("RCM Flow (*.rcmflow)"));
    if (path.isEmpty()) return;
    openEditor(path);
}

void MainWindow::saveFlow() {
    FlowEditorView* editor = currentEditor();
    if (editor) editor->save();
}

void MainWindow::saveFlowAs() {
    FlowEditorView* editor = currentEditor();
    if (!editor) return;
    const QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("Salva con nome"), editor->filePath(),
        QStringLiteral("RCM Flow (*.rcmflow)"));
    if (path.isEmpty()) return;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::critical(this, QStringLiteral("Errore"),
                              QStringLiteral("Impossibile salvare il flusso: ") + file.errorString());
        return;
    }
    file.write(FlowFileManager::toJson(editor->graph()).toUtf8());
    file.close();
}

void MainWindow::runFlow() {
    FlowEditorView* editor = currentEditor();
    if (editor) editor->runAll();
}

} // namespace rcm
