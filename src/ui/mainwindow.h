#pragma once

#include <QMainWindow>

class QTabWidget;
class QDockWidget;
class QAction;

namespace rcm {

class FlowEditorView;
class RestToolWindow;
class ExplorerPanel;

class MainWindow : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    void newFlow();
    void openFlow();
    void saveFlow();
    void saveFlowAs();
    void runFlow();

    FlowEditorView* currentEditor() const;
    FlowEditorView* openEditor(const QString& filePath);
    void saveSession();
    void restoreSession();
    void refreshExplorer();

    QTabWidget* tabs_ = nullptr;
    RestToolWindow* toolWindow_ = nullptr;
    QDockWidget* dock_ = nullptr;
    QDockWidget* explorerDock_ = nullptr;
    ExplorerPanel* explorer_ = nullptr;
    QAction* runAction_ = nullptr;
};

} // namespace rcm
