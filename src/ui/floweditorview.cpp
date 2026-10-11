#include "floweditorview.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QFile>
#include <QInputDialog>
#include <QMessageBox>
#include <QShowEvent>
#include <QFileInfo>
#include <QPalette>
#include "widgets/appicons.h"
#include "../flow/flowfilemanager.h"
#include "../flow/rcmflowrunservice.h"
#include "visual/visualrequestview.h"
#include "dialogs/flownodedialog.h"
#include "dialogs/flowsoapnodedialog.h"
#include "dialogs/flowedgedialog.h"
#include "dialogs/flowvariablesdialog.h"
#include "dialogs/flowcollectionvariablesdialog.h"
#include "dialogs/flowvariablenodedialog.h"

namespace rcm {

FlowEditorView::FlowEditorView(const QString& filePath, QWidget* parent)
    : QWidget(parent), filePath_(filePath), fileName_(QFileInfo(filePath).fileName()) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    QWidget* toolbar = new QWidget(this);
    QHBoxLayout* toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(4, 4, 4, 4);
    const QColor iconColor = palette().color(QPalette::ButtonText);
    QPushButton* variablesButton = new QPushButton(QStringLiteral("Variabili"), toolbar);
    variablesButton->setIcon(globeIcon(iconColor));
    variablesButton->setToolTip(QStringLiteral("Variabili globali del flusso"));
    toolbarLayout->addWidget(variablesButton);
    toolbarLayout->addStretch();

    graphButton_ = new QToolButton(toolbar);
    graphButton_->setIcon(graphIcon(iconColor));
    graphButton_->setToolTip(QStringLiteral("Vista grafo"));
    graphButton_->setCheckable(true);
    graphButton_->setChecked(true);
    graphButton_->setAutoRaise(true);
    visualButton_ = new QToolButton(toolbar);
    visualButton_->setIcon(visualIcon(iconColor));
    visualButton_->setToolTip(QStringLiteral("Vista visuale (singola chiamata)"));
    visualButton_->setCheckable(true);
    visualButton_->setAutoRaise(true);
    toolbarLayout->addWidget(graphButton_);
    toolbarLayout->addWidget(visualButton_);
    layout->addWidget(toolbar);
    connect(variablesButton, &QPushButton::clicked, this, [this]() { editGlobalVariables(); });
    connect(graphButton_, &QToolButton::clicked, this, [this]() { showGraphView(); });
    connect(visualButton_, &QToolButton::clicked, this, [this]() { showVisualView(); });

    canvas_ = new FlowCanvas(this);
    visualView_ = new VisualRequestView(this);
    stack_ = new QStackedWidget(this);
    stack_->addWidget(canvas_);
    stack_->addWidget(visualView_);
    layout->addWidget(stack_, 1);

    statusLabel_ = new QLabel(QStringLiteral(" "), this);
    statusLabel_->setContentsMargins(6, 2, 6, 2);
    layout->addWidget(statusLabel_);

    saveTimer_ = new QTimer(this);
    saveTimer_->setSingleShot(true);
    saveTimer_->setInterval(2000);
    connect(saveTimer_, &QTimer::timeout, this, [this]() { save(); });

    FlowCanvas::Listener listener;
    listener.graphChanged = [this]() {
        canvas_->setResults({});
        markModified();
    };
    listener.selectionChanged = [this]() {
        if (onSelectionChanged) onSelectionChanged();
    };
    listener.editNode = [this](const FlowNode& node) { openNodeDialog(node); };
    listener.editEdge = [this](const FlowEdge& edge) { editEdge(edge); };
    listener.addNodeRequested = [this]() { addRestNode(); };
    listener.addSoapNodeRequested = [this]() { addSoapNode(); };
    listener.addVariableNodeRequested = [this]() { addVariableNode(); };
    listener.addGroupRequested = [this]() { addGroup(); };
    listener.editGlobalVariablesRequested = [this]() { editGlobalVariables(); };
    listener.renameGroupRequested = [this](const FlowGroup& g) { renameGroup(g); };
    listener.editGroupVariablesRequested = [this](const FlowGroup& g) { editGroupVariables(g); };
    listener.createEdgeRequested = [this](const QString& from, const QString& to) {
        createEdge(from, to);
    };
    listener.runAllRequested = [this]() { runAll(); };
    listener.runFromNodeRequested = [this](const QString& id) { runFrom(id); };
    canvas_->setListener(listener);

    flowListenerId_ = RcmFlowRunService::instance().addListener([this]() { onResultsChanged(); });
}

FlowEditorView::~FlowEditorView() {
    saveTimer_->stop();
    if (modified_) writeToFile();
    RcmFlowRunService::instance().removeListener(flowListenerId_);
}

void FlowEditorView::load() {
    QFile file(filePath_);
    if (!file.open(QIODevice::ReadOnly)) return;
    const QString text = QString::fromUtf8(file.readAll());
    file.close();
    canvas_->setGraph(FlowFileManager::load(text));
}

void FlowEditorView::selectNode(const QString& id) {
    showGraphView();
    canvas_->selectNode(id);
    canvas_->centerOnNode(id);
}

void FlowEditorView::selectGroup(const QString& id) {
    showGraphView();
    canvas_->selectGroup(id);
    canvas_->centerOnGroup(id);
}

void FlowEditorView::showGraphView() {
    stack_->setCurrentWidget(canvas_);
    graphButton_->setChecked(true);
    visualButton_->setChecked(false);
}

void FlowEditorView::showVisualView() {
    const QString nodeId = canvas_->getSelectedNodeId();
    const FlowNode* node = canvas_->graph().nodeById(nodeId);
    if (node) {
        visualView_->prefillFrom(*node, canvas_->graph(), availableVariables(node));
    }
    stack_->setCurrentWidget(visualView_);
    visualButton_->setChecked(true);
    graphButton_->setChecked(false);
}

void FlowEditorView::setModified(bool modified) {
    modified_ = modified;
}

void FlowEditorView::markModified() {
    modified_ = true;
    canvas_->update();
    saveTimer_->start();
    if (onStructureChanged) onStructureChanged();
}

void FlowEditorView::writeToFile() {
    const QString json = FlowFileManager::toJson(canvas_->graph());
    QFile file(filePath_);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::critical(this, QStringLiteral("Errore"),
                              QStringLiteral("Impossibile salvare il flusso: ") +
                                  file.errorString());
        return;
    }
    file.write(json.toUtf8());
    file.close();
    modified_ = false;
}

void FlowEditorView::save() {
    if (!modified_) return;
    writeToFile();
}

void FlowEditorView::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    if (!autoFitted_ && width() > 0 && height() > 0) {
        autoFitted_ = true;
        canvas_->fitView();
    }
}

QStringList FlowEditorView::availableVariables(const FlowNode* node) const {
    const FlowGraph& graph = canvas_->graph();
    QStringList names;
    QSet<QString> seen;

    auto addName = [&](const QString& name) {
        if (!seen.contains(name)) {
            seen.insert(name);
            names << name;
        }
    };

    for (auto it = graph.variables().begin(); it != graph.variables().end(); ++it) {
        addName(s2q(it.key()));
    }

    if (node) {
        if (const FlowGroup* group = graph.groupOf(node->id)) {
            for (const auto& kv : group->variables) addName(kv.first);
        }
        for (const auto& edge : graph.incomingEdges(node->id)) {
            const FlowNode* source = graph.nodeById(edge.from);
            if (!source) continue;
            if (source->isVariableNode()) {
                for (const auto& kv : source->vars) addName(kv.first);
            } else if (edge.outputs) {
                for (const auto& out : *edge.outputs) {
                    if (!out.name.trimmed().isEmpty()) addName(out.name);
                }
            }
        }
    }
    return names;
}

QString FlowEditorView::nextNodeId() const {
    int maxId = 0;
    for (const auto& n : canvas_->graph().nodes()) {
        if (n.id.startsWith(QLatin1Char('n'))) {
            bool ok = false;
            const int v = n.id.mid(1).toInt(&ok);
            if (ok) maxId = std::max(maxId, v);
        }
    }
    return QStringLiteral("n") + QString::number(maxId + 1);
}

QString FlowEditorView::nextEdgeId() const {
    int maxId = 0;
    for (const auto& e : canvas_->graph().edges()) {
        if (e.id.startsWith(QLatin1Char('e'))) {
            bool ok = false;
            const int v = e.id.mid(1).toInt(&ok);
            if (ok) maxId = std::max(maxId, v);
        }
    }
    return QStringLiteral("e") + QString::number(maxId + 1);
}

void FlowEditorView::addRestNode() {
    const QStringList vars = availableVariables(nullptr);
    FlowNodeDialog dialog(vars, Json(), false, this);
    if (dialog.exec() != QDialog::Accepted) return;
    const Json call = dialog.getCall();
    const QString title = dialog.getNodeTitle();
    const QPoint center = canvas_->viewCenterFlow();
    const int off = (static_cast<int>(canvas_->graph().nodes().size()) % 6) * 30;
    const QString id = nextNodeId();
    canvas_->graph().nodes().push_back(
        FlowNode::rest(id, center.x() - 110 + off, center.y() - 39 + off, title, call));
    canvas_->selectNode(id);
    markModified();
}

void FlowEditorView::addSoapNode() {
    const QStringList vars = availableVariables(nullptr);
    FlowSoapNodeDialog dialog(vars, Json(), false, this);
    if (dialog.exec() != QDialog::Accepted) return;
    const Json call = dialog.getCall();
    const QString title = dialog.getNodeTitle();
    const QPoint center = canvas_->viewCenterFlow();
    const int off = (static_cast<int>(canvas_->graph().nodes().size()) % 6) * 30;
    const QString id = nextNodeId();
    canvas_->graph().nodes().push_back(FlowNode(id, center.x() - 110 + off,
                                                center.y() - 39 + off, title,
                                                FlowNode::KIND_SOAP, call, {}));
    canvas_->selectNode(id);
    markModified();
}

void FlowEditorView::addVariableNode() {
    FlowVariableNodeDialog dialog(QString(), {}, false, this);
    if (dialog.exec() != QDialog::Accepted) return;
    const QPoint center = canvas_->viewCenterFlow();
    const int off = (static_cast<int>(canvas_->graph().nodes().size()) % 6) * 30;
    const QString id = nextNodeId();
    canvas_->graph().nodes().push_back(
        FlowNode::variables(id, center.x() - 110 + off, center.y() - 39 + off,
                            dialog.getNodeTitle(), dialog.getVariables()));
    canvas_->selectNode(id);
    markModified();
}

void FlowEditorView::addGroup() {
    bool ok = false;
    const QString name = QInputDialog::getText(
        this, QStringLiteral("Nuova collection"), QStringLiteral("Nome della collection:"),
        QLineEdit::Normal,
        QStringLiteral("Collection ") + QString::number(canvas_->graph().groups().size() + 1), &ok);
    if (!ok || name.trimmed().isEmpty()) return;
    canvas_->addGroup(name.trimmed());
}

void FlowEditorView::editGlobalVariables() {
    FlowVariablesDialog dialog(canvas_->graph().variables(), this);
    if (dialog.exec() != QDialog::Accepted) return;
    const VarMap values = dialog.getVariables();
    Json obj = Json::object();
    for (const auto& kv : values) obj[q2s(kv.first)] = q2s(kv.second);
    canvas_->graph().variables() = obj;
    markModified();
}

void FlowEditorView::renameGroup(const FlowGroup& group) {
    bool ok = false;
    const QString name = QInputDialog::getText(this, QStringLiteral("Rinomina collection"),
                                               QStringLiteral("Nome della collection:"),
                                               QLineEdit::Normal, group.name, &ok);
    if (!ok || name.trimmed().isEmpty()) return;
    canvas_->graph().renameGroup(group.id, name.trimmed());
    markModified();
}

void FlowEditorView::editGroupVariables(const FlowGroup& group) {
    FlowCollectionVariablesDialog dialog(group.name, group.variables, this);
    if (dialog.exec() != QDialog::Accepted) return;
    canvas_->graph().updateGroupVariables(group.id, dialog.getVariables());
    markModified();
}

void FlowEditorView::createEdge(const QString& fromId, const QString& toId) {
    const FlowNode* from = canvas_->graph().nodeById(fromId);
    const FlowNode* to = canvas_->graph().nodeById(toId);
    if (!from || !to) return;
    const CallResult* lastResult = nullptr;
    const auto results = RcmFlowRunService::instance().getResults();
    auto it = results.find(fromId);
    if (it != results.end()) lastResult = &it->second;

    FlowEdgeDialog dialog(*from, to->title, {}, lastResult, true, this);
    if (dialog.exec() != QDialog::Accepted) return;
    FlowEdge edge(nextEdgeId(), fromId, toId, dialog.getEdgeOutputs());
    canvas_->graph().edges().push_back(edge);
    markModified();
}

void FlowEditorView::editEdge(const FlowEdge& edge) {
    const FlowNode* from = canvas_->graph().nodeById(edge.from);
    const FlowNode* to = canvas_->graph().nodeById(edge.to);
    if (!from || !to) return;
    const CallResult* lastResult = nullptr;
    const auto results = RcmFlowRunService::instance().getResults();
    auto it = results.find(edge.from);
    if (it != results.end()) lastResult = &it->second;

    FlowEdgeDialog dialog(*from, to->title, edge.outputs ? *edge.outputs : std::vector<FlowOutput>{},
                          lastResult, false, this);
    if (dialog.exec() != QDialog::Accepted) return;
    FlowEdge updated = edge;
    updated.outputs = dialog.getEdgeOutputs();
    for (auto& e : canvas_->graph().edges()) {
        if (e.id == edge.id) {
            e = updated;
            break;
        }
    }
    markModified();
}

void FlowEditorView::openNodeDialog(const FlowNode& node) {
    if (node.isVariableNode()) {
        FlowVariableNodeDialog dialog(node.title, node.vars, true, this);
        if (dialog.exec() != QDialog::Accepted) return;
        FlowNode updated = node;
        updated.title = dialog.getNodeTitle();
        updated.vars = dialog.getVariables();
        canvas_->graph().replaceNode(updated);
        markModified();
    } else if (node.isSoapNode()) {
        const QStringList vars = availableVariables(&node);
        FlowSoapNodeDialog dialog(vars, node.call, true, this);
        if (dialog.exec() != QDialog::Accepted) return;
        FlowNode updated = node;
        updated.title = dialog.getNodeTitle();
        updated.call = dialog.getCall();
        canvas_->graph().replaceNode(updated);
        markModified();
    } else {
        const QStringList vars = availableVariables(&node);
        FlowNodeDialog dialog(vars, node.call, true, this);
        if (dialog.exec() != QDialog::Accepted) return;
        FlowNode updated = node;
        updated.title = dialog.getNodeTitle();
        updated.call = dialog.getCall();
        canvas_->graph().replaceNode(updated);
        markModified();
    }
}

void FlowEditorView::runAll() {
    save();
    RcmFlowRunService::instance().runAll(canvas_->graph(), fileName_);
    if (activateToolWindow) activateToolWindow();
}

void FlowEditorView::runFrom(const QString& nodeId) {
    save();
    RcmFlowRunService::instance().runFrom(canvas_->graph(), nodeId, fileName_);
    if (activateToolWindow) activateToolWindow();
}

void FlowEditorView::onResultsChanged() {
    canvas_->setResults(RcmFlowRunService::instance().getResults());
    const auto results = RcmFlowRunService::instance().getResults();
    if (results.empty()) {
        statusLabel_->setText(QStringLiteral(" "));
        return;
    }
    int okCount = 0;
    for (const auto& kv : results) {
        if (kv.second.errorMessage.isEmpty() && kv.second.success) ++okCount;
    }
    statusLabel_->setText(QStringLiteral(" Ultima esecuzione: ") +
                          QString::number(static_cast<int>(results.size())) +
                          QStringLiteral(" chiamate, ") + QString::number(okCount) +
                          QStringLiteral(" OK "));
}

} // namespace rcm
