#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <QStackedWidget>
#include <QToolButton>
#include <functional>
#include "flowcanvas.h"
#include "../flow/flowgraph.h"

class QTimer;

namespace rcm {

class VisualRequestView;

class FlowEditorView : public QWidget {
public:
    explicit FlowEditorView(const QString& filePath, QWidget* parent = nullptr);
    ~FlowEditorView() override;

    void load();
    void save();
    void runAll();

    QString filePath() const { return filePath_; }
    QString fileName() const { return fileName_; }
    bool isModified() const { return modified_; }
    void setModified(bool modified);
    FlowGraph& graph() { return canvas_->graph(); }

    void selectNode(const QString& id);
    void selectGroup(const QString& id);
    QString selectedNodeId() const { return canvas_->getSelectedNodeId(); }
    QString selectedGroupId() const { return canvas_->getSelectedGroupId(); }

    void addRestNode();
    void addSoapNode();
    void addVariableNode();
    void addGroup();

    std::function<void()> activateToolWindow;
    std::function<void()> onStructureChanged;
    std::function<void()> onSelectionChanged;

protected:
    void showEvent(QShowEvent* event) override;

private:
    void markModified();
    void writeToFile();
    QStringList availableVariables(const FlowNode* node) const;

    QString nextNodeId() const;
    QString nextEdgeId() const;

    void openNodeDialog(const FlowNode& node);
    void editGlobalVariables();
    void renameGroup(const FlowGroup& group);
    void editGroupVariables(const FlowGroup& group);
    void createEdge(const QString& fromId, const QString& toId);
    void editEdge(const FlowEdge& edge);
    void runFrom(const QString& nodeId);
    void onResultsChanged();

    void showGraphView();
    void showVisualView();

    FlowCanvas* canvas_ = nullptr;
    QStackedWidget* stack_ = nullptr;
    VisualRequestView* visualView_ = nullptr;
    QToolButton* graphButton_ = nullptr;
    QToolButton* visualButton_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QString filePath_;
    QString fileName_;
    bool modified_ = false;
    bool autoFitted_ = false;
    QTimer* saveTimer_ = nullptr;
    int flowListenerId_ = -1;
};

} // namespace rcm
