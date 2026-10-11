#pragma once

#include <QWidget>
#include <QPointF>
#include <functional>
#include <map>
#include "../flow/flowgraph.h"
#include "../http/restclient.h"

namespace rcm {

class FlowCanvas : public QWidget {
public:
    struct Listener {
        std::function<void()> graphChanged;
        std::function<void()> selectionChanged;
        std::function<void(const FlowNode&)> editNode;
        std::function<void(const FlowEdge&)> editEdge;
        std::function<void()> addNodeRequested;
        std::function<void()> addSoapNodeRequested;
        std::function<void()> addVariableNodeRequested;
        std::function<void()> addGroupRequested;
        std::function<void()> editGlobalVariablesRequested;
        std::function<void(const FlowGroup&)> renameGroupRequested;
        std::function<void(const FlowGroup&)> editGroupVariablesRequested;
        std::function<void(const QString&, const QString&)> createEdgeRequested;
        std::function<void()> runAllRequested;
        std::function<void(const QString&)> runFromNodeRequested;
    };

    explicit FlowCanvas(QWidget* parent = nullptr);

    void setListener(const Listener& listener) { listener_ = listener; }
    FlowGraph& graph() { return graph_; }
    const FlowGraph& graph() const { return graph_; }
    void setGraph(const FlowGraph& graph);
    void setResults(const std::map<QString, CallResult>& results);
    void selectNode(const QString& id);
    void selectGroup(const QString& id);
    QString getSelectedNodeId() const { return selectedNodeId_; }
    QString getSelectedGroupId() const { return selectedGroupId_; }

    void fitView();
    void addGroup(const QString& name);
    void duplicateSelectedNode();
    void deleteSelection();
    QPoint viewCenterFlow() const;
    void centerOnNode(const QString& id);
    void centerOnGroup(const QString& id);

    QSize sizeHint() const override { return QSize(900, 600); }

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    QPointF toFlow(const QPoint& view) const;
    QPointF toView(const QPointF& flow) const;

    int nodeWidth(const FlowNode& node) const;
    int nodeHeight(const FlowNode& node) const;
    QPointF outputPoint(const FlowNode& node) const;
    QPointF inputPoint(const FlowNode& node) const;

    const FlowNode* nodeAt(const QPointF& p) const;
    const FlowEdge* edgeAt(const QPointF& p) const;
    const FlowGroup* groupHeaderAt(const QPointF& p) const;
    const FlowGroup* groupResizeCornerAt(const QPointF& p) const;
    const FlowGroup* groupContainingNode(const FlowNode& node) const;
    const FlowNode* portAt(const QPointF& p, bool output) const;

    int snap(int v) const;
    double safeZoom() const { return zoom_ > 0.05 ? zoom_ : 1.0; }

    QColor methodColor(const QString& method) const;
    QFont methodFont() const;
    QFont badgeFont() const;
    QFont urlFont() const;
    QFont variableNameFont() const;
    QFont variableValueFont() const;
    QFont groupFont() const;
    QFont edgeFont() const;

    void drawGroup(QPainter& painter, const FlowGroup& group);
    void drawEdge(QPainter& painter, const FlowEdge& edge);
    void drawNode(QPainter& painter, const FlowNode& node);
    void drawVariableRows(QPainter& painter, const FlowNode& node);
    void drawRestBody(QPainter& painter, const FlowNode& node);
    void drawPort(QPainter& painter, const QPointF& p, const QColor& color);
    void drawRubberBand(QPainter& painter);
    void drawIcon(QPainter& painter, const QRectF& rect, bool soap);
    QString edgeLabel(const FlowEdge& edge) const;

    static QString fit(const QString& text, const QFontMetrics& fm, int maxWidth);

    void buildNodeMenu(QMenu& menu, const FlowNode& node);
    void buildEdgeMenu(QMenu& menu, const FlowEdge& edge);
    void buildGroupMenu(QMenu& menu, const FlowGroup& group);
    void buildEmptyMenu(QMenu& menu);

    void duplicateNode(const FlowNode& node);
    void deleteNode(const QString& id);
    void deleteEdge(const QString& id);
    void deleteGroup(const QString& id);

    FlowGraph graph_;
    std::map<QString, CallResult> results_;
    Listener listener_;

    QPointF pan_{60, 40};
    double zoom_ = 1.0;

    QString selectedNodeId_;
    QString selectedEdgeId_;
    QString selectedGroupId_;

    enum DragMode { NONE, NODE, GROUP, RESIZE, CONNECT, PAN };
    DragMode dragMode_ = NONE;
    QPoint dragStartMouse_;
    QPointF dragStartPan_;
    QPoint dragStartNodePos_;
    QPoint dragStartGroupPos_;
    QString connectingFromId_;
    QPointF connectCursor_;
    QString resizingGroupId_;
    QString hoverGroupId_;

    static constexpr int NODE_W = 220;
    static constexpr int NODE_H = 78;
    static constexpr int HEADER_H = 26;
    static constexpr int VAR_ROW_H = 20;
    static constexpr int VAR_MAX_ROWS = 10;
    static constexpr int VAR_VALUE_MIN_W = 90;
    static constexpr int NODE_PAD = 10;
    static constexpr int GROUP_HEADER_H = 30;
    static constexpr int GROUP_MIN_W = 180;
    static constexpr int GROUP_MIN_H = 110;
    static constexpr int PORT_R = 7;
    static constexpr int GRID = 10;
};

} // namespace rcm
