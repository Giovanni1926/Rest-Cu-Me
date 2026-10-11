#include "flowcanvas.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QFontMetrics>
#include <QApplication>
#include <cmath>
#include <algorithm>

namespace rcm {

static const QColor COLOR_GET(0x4C, 0xAF, 0x50);
static const QColor COLOR_POST(0xFF, 0x98, 0x00);
static const QColor COLOR_PUT(0x21, 0x96, 0xF3);
static const QColor COLOR_PATCH(0x00, 0x96, 0x88);
static const QColor COLOR_DELETE(0xF4, 0x43, 0x36);
static const QColor COLOR_SUCCESS(0x4C, 0xAF, 0x50);
static const QColor COLOR_ERROR(0xF4, 0x43, 0x36);
static const QColor COLOR_GROUP(0x78, 0x90, 0x9C);
static const QColor COLOR_VARIABLES(0x7E, 0x57, 0xC2);
static const QColor COLOR_SOAP(0x00, 0x83, 0x8F);

FlowCanvas::FlowCanvas(QWidget* parent) : QWidget(parent) {
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setContextMenuPolicy(Qt::DefaultContextMenu);
    setMinimumSize(200, 200);
}

void FlowCanvas::setGraph(const FlowGraph& graph) {
    graph_ = graph;
    selectedNodeId_.clear();
    selectedEdgeId_.clear();
    selectedGroupId_.clear();
    update();
}

void FlowCanvas::setResults(const std::map<QString, CallResult>& results) {
    results_ = results;
    update();
}

void FlowCanvas::selectNode(const QString& id) {
    selectedNodeId_ = id;
    selectedEdgeId_.clear();
    selectedGroupId_.clear();
    update();
    if (listener_.selectionChanged) listener_.selectionChanged();
}

void FlowCanvas::selectGroup(const QString& id) {
    selectedGroupId_ = id;
    selectedNodeId_.clear();
    selectedEdgeId_.clear();
    update();
    if (listener_.selectionChanged) listener_.selectionChanged();
}

QPointF FlowCanvas::toFlow(const QPoint& view) const {
    const double z = safeZoom();
    return QPointF((view.x() - pan_.x()) / z, (view.y() - pan_.y()) / z);
}

QPointF FlowCanvas::toView(const QPointF& flow) const {
    return QPointF(flow.x() * zoom_ + pan_.x(), flow.y() * zoom_ + pan_.y());
}

int FlowCanvas::snap(int v) const {
    return static_cast<int>(std::round(v / static_cast<double>(GRID))) * GRID;
}

QPoint FlowCanvas::viewCenterFlow() const {
    const QPointF f = toFlow(QPoint(width() / 2, height() / 2));
    return QPoint(snap(static_cast<int>(f.x())), snap(static_cast<int>(f.y())));
}

void FlowCanvas::centerOnNode(const QString& id) {
    const FlowNode* n = graph_.nodeById(id);
    if (!n) return;
    const double cx = n->x + nodeWidth(*n) / 2.0;
    const double cy = n->y + nodeHeight(*n) / 2.0;
    pan_ = QPointF(width() / 2.0 - cx * zoom_, height() / 2.0 - cy * zoom_);
    update();
}

void FlowCanvas::centerOnGroup(const QString& id) {
    const FlowGroup* g = graph_.groupById(id);
    if (!g) return;
    const double cx = g->x + g->w / 2.0;
    const double cy = g->y + g->h / 2.0;
    pan_ = QPointF(width() / 2.0 - cx * zoom_, height() / 2.0 - cy * zoom_);
    update();
}

static int clampCoord(int v) {
    constexpr int kLimit = 1000000;
    if (v < -kLimit) return -kLimit;
    if (v > kLimit) return kLimit;
    return v;
}

QFont FlowCanvas::methodFont() const {
    QFont f;
    f.setBold(true);
    f.setPointSize(10);
    return f;
}
QFont FlowCanvas::badgeFont() const {
    QFont f;
    f.setBold(true);
    f.setPointSize(8);
    return f;
}
QFont FlowCanvas::urlFont() const {
    QFont f;
    f.setPointSize(9);
    return f;
}
QFont FlowCanvas::variableNameFont() const {
    QFont f;
    f.setBold(true);
    f.setPointSize(9);
    return f;
}
QFont FlowCanvas::variableValueFont() const {
    QFont f;
    f.setPointSize(9);
    return f;
}
QFont FlowCanvas::groupFont() const {
    QFont f;
    f.setBold(true);
    f.setPointSize(11);
    return f;
}
QFont FlowCanvas::edgeFont() const {
    QFont f;
    f.setPointSize(8);
    return f;
}

QColor FlowCanvas::methodColor(const QString& method) const {
    if (method == QStringLiteral("GET")) return COLOR_GET;
    if (method == QStringLiteral("POST")) return COLOR_POST;
    if (method == QStringLiteral("PUT")) return COLOR_PUT;
    if (method == QStringLiteral("PATCH")) return COLOR_PATCH;
    if (method == QStringLiteral("DELETE")) return COLOR_DELETE;
    return COLOR_GET;
}

QString FlowCanvas::fit(const QString& text, const QFontMetrics& fm, int maxWidth) {
    if (fm.horizontalAdvance(text) <= maxWidth) return text;
    if (maxWidth < fm.horizontalAdvance(QStringLiteral("\u2026"))) return QString();
    QString t = text;
    while (!t.isEmpty() && fm.horizontalAdvance(t + QStringLiteral("\u2026")) > maxWidth) {
        t.chop(1);
    }
    return t + QStringLiteral("\u2026");
}

int FlowCanvas::nodeWidth(const FlowNode& node) const {
    if (!node.isVariableNode()) return NODE_W;
    QFontMetrics fm(variableNameFont());
    int maxName = 0;
    for (const auto& kv : node.vars) {
        maxName = std::max(maxName, fm.horizontalAdvance(QStringLiteral("${") + kv.first + QStringLiteral("}")));
    }
    return std::max(NODE_W, maxName + NODE_PAD * 2 + VAR_VALUE_MIN_W);
}

int FlowCanvas::nodeHeight(const FlowNode& node) const {
    if (!node.isVariableNode()) return NODE_H;
    const int count = static_cast<int>(node.vars.size());
    const int rows = count == 0 ? 1 : std::min(count, VAR_MAX_ROWS) + (count > VAR_MAX_ROWS ? 1 : 0);
    return HEADER_H + 6 + rows * VAR_ROW_H + 6;
}

QPointF FlowCanvas::outputPoint(const FlowNode& node) const {
    const int w = nodeWidth(node);
    const int h = nodeHeight(node);
    if (node.isVariableNode()) return QPointF(node.x + w / 2.0, node.y + h);
    return QPointF(node.x + w, node.y + h / 2.0);
}

QPointF FlowCanvas::inputPoint(const FlowNode& node) const {
    return QPointF(node.x, node.y + nodeHeight(node) / 2.0);
}

const FlowNode* FlowCanvas::nodeAt(const QPointF& p) const {
    for (auto it = graph_.nodes().rbegin(); it != graph_.nodes().rend(); ++it) {
        const QRectF rect(it->x, it->y, nodeWidth(*it), nodeHeight(*it));
        if (rect.contains(p)) return &(*it);
    }
    return nullptr;
}

const FlowNode* FlowCanvas::portAt(const QPointF& p, bool output) const {
    for (auto it = graph_.nodes().rbegin(); it != graph_.nodes().rend(); ++it) {
        if (!output && it->isVariableNode()) continue;
        const QPointF port = output ? outputPoint(*it) : inputPoint(*it);
        const double dx = p.x() - port.x();
        const double dy = p.y() - port.y();
        if (std::sqrt(dx * dx + dy * dy) <= PORT_R + 7) return &(*it);
    }
    return nullptr;
}

const FlowGroup* FlowCanvas::groupHeaderAt(const QPointF& p) const {
    for (auto it = graph_.groups().rbegin(); it != graph_.groups().rend(); ++it) {
        if (p.x() >= it->x && p.x() <= it->x + it->w && p.y() >= it->y &&
            p.y() <= it->y + GROUP_HEADER_H) {
            return &(*it);
        }
    }
    return nullptr;
}

const FlowGroup* FlowCanvas::groupResizeCornerAt(const QPointF& p) const {
    for (auto it = graph_.groups().rbegin(); it != graph_.groups().rend(); ++it) {
        const double dx = p.x() - (it->x + it->w);
        const double dy = p.y() - (it->y + it->h);
        if (std::sqrt(dx * dx + dy * dy) <= 12) return &(*it);
    }
    return nullptr;
}

const FlowGroup* FlowCanvas::groupContainingNode(const FlowNode& node) const {
    const double cx = node.x + nodeWidth(node) / 2.0;
    const double cy = node.y + nodeHeight(node) / 2.0;
    for (const auto& g : graph_.groups()) {
        if (cx >= g.x && cx <= g.x + g.w && cy >= g.y && cy <= g.y + g.h) return &g;
    }
    return nullptr;
}

const FlowEdge* FlowCanvas::edgeAt(const QPointF& p) const {
    auto distToSeg = [](const QPointF& p, const QPointF& a, const QPointF& b) {
        const double dx = b.x() - a.x();
        const double dy = b.y() - a.y();
        if (dx == 0 && dy == 0) return std::sqrt(std::pow(p.x() - a.x(), 2) + std::pow(p.y() - a.y(), 2));
        const double t = std::max(0.0, std::min(1.0, ((p.x() - a.x()) * dx + (p.y() - a.y()) * dy) / (dx * dx + dy * dy)));
        const double px = a.x() + t * dx;
        const double py = a.y() + t * dy;
        return std::sqrt(std::pow(p.x() - px, 2) + std::pow(p.y() - py, 2));
    };
    for (const auto& e : graph_.edges()) {
        const FlowNode* from = graph_.nodeById(e.from);
        const FlowNode* to = graph_.nodeById(e.to);
        if (!from || !to) continue;
        const QPointF start = outputPoint(*from);
        const QPointF end = inputPoint(*to);
        const QPointF mid((start.x() + end.x()) / 2.0, (start.y() + end.y()) / 2.0);
        if (from->isVariableNode()) {
            const double c = std::max(30.0, std::abs(end.y() - start.y()) * 0.5);
            if (distToSeg(p, start, QPointF(start.x(), start.y() + c)) <= 7) return &e;
            if (distToSeg(p, QPointF(end.x(), end.y() - c), end) <= 7) return &e;
        } else {
            if (distToSeg(p, start, mid) <= 7) return &e;
            if (distToSeg(p, mid, end) <= 7) return &e;
        }
    }
    return nullptr;
}

void FlowCanvas::drawIcon(QPainter& painter, const QRectF& rect, bool soap) {
    painter.save();
    if (soap) {
        painter.setPen(Qt::white);
        painter.setBrush(Qt::white);
        const double cx = rect.center().x();
        const double cy = rect.center().y();
        painter.drawRoundedRect(rect.adjusted(1, 1, -1, -1), 3, 3);
        painter.setBrush(QColor(0x00, 0x83, 0x8F));
        painter.drawEllipse(QPointF(cx - rect.width() * 0.12, cy - rect.height() * 0.1),
                            rect.width() * 0.14, rect.width() * 0.14);
        painter.drawEllipse(QPointF(cx + rect.width() * 0.10, cy + rect.height() * 0.14),
                            rect.width() * 0.18, rect.width() * 0.18);
        painter.drawEllipse(QPointF(cx + rect.width() * 0.22, cy - rect.height() * 0.16),
                            rect.width() * 0.09, rect.width() * 0.09);
    } else {
        painter.setPen(Qt::white);
        painter.setBrush(Qt::NoBrush);
        const QRectF circle = rect.adjusted(2, 2, -2, -2);
        painter.drawEllipse(circle);
        painter.drawEllipse(circle.adjusted(circle.width() * 0.35, 0, -circle.width() * 0.35, 0));
        const double cy = circle.center().y();
        painter.drawLine(QPointF(circle.left(), cy), QPointF(circle.right(), cy));
    }
    painter.restore();
}

QString FlowCanvas::edgeLabel(const FlowEdge& edge) const {
    const FlowNode* from = graph_.nodeById(edge.from);
    QStringList names;
    if (from && from->isVariableNode()) {
        for (const auto& kv : from->vars) names << kv.first;
    } else if (edge.outputs) {
        for (const auto& o : *edge.outputs) names << o.name;
    }
    if (names.isEmpty()) return QStringLiteral("\u2205");
    return names.join(QStringLiteral(", "));
}

void FlowCanvas::drawGroup(QPainter& painter, const FlowGroup& group) {
    const QColor body = COLOR_GROUP;
    QColor bodyFill = body;
    bodyFill.setAlpha(20);
    QColor headerFill = body;
    headerFill.setAlpha(60);
    QColor border = body;
    border.setAlpha(160);

    painter.setPen(Qt::NoPen);
    painter.setBrush(bodyFill);
    painter.drawRoundedRect(group.x, group.y, group.w, group.h, 14, 14);

    painter.setBrush(headerFill);
    painter.drawRoundedRect(group.x, group.y, group.w, GROUP_HEADER_H + 10, 14, 14);
    painter.drawRect(group.x, group.y + GROUP_HEADER_H - 6, group.w, 8);

    const bool selected = selectedGroupId_ == group.id;
    const bool hoverDrop = hoverGroupId_ == group.id;

    painter.setBrush(Qt::NoBrush);
    if (selected) {
        painter.setPen(QPen(palette().highlight(), 2.0));
    } else if (hoverDrop) {
        painter.setPen(QPen(COLOR_SUCCESS, 2.6));
    } else {
        painter.setPen(QPen(border, 1.2));
    }
    painter.drawRoundedRect(group.x, group.y, group.w, group.h, 14, 14);

    painter.setFont(groupFont());
    painter.setPen(palette().windowText().color());
    const QFontMetrics fm = painter.fontMetrics();
    const QString name = fit(group.name, fm, group.w - 16);
    const QRectF textRect(group.x, group.y, group.w, GROUP_HEADER_H);
    painter.save();
    painter.setClipRect(textRect);
    painter.drawText(QRectF(group.x, group.y, group.w, GROUP_HEADER_H),
                     Qt::AlignCenter, name);
    painter.restore();

    if (selected) {
        painter.fillRect(QRectF(group.x + group.w - 6, group.y + group.h - 6, 12, 12),
                         palette().highlight());
    }
}

void FlowCanvas::drawEdge(QPainter& painter, const FlowEdge& edge) {
    const FlowNode* from = graph_.nodeById(edge.from);
    const FlowNode* to = graph_.nodeById(edge.to);
    if (!from || !to) return;

    const QPointF start = outputPoint(*from);
    const QPointF end = inputPoint(*to);
    const bool selected = selectedEdgeId_ == edge.id;
    const QColor color = selected ? palette().highlight().color() : palette().windowText().color();

    QPainterPath path;
    QPointF control;
    if (from->isVariableNode()) {
        const double c = std::max(30.0, std::abs(end.y() - start.y()) * 0.5);
        path.moveTo(start);
        path.cubicTo(start.x(), start.y() + c, end.x(), end.y() - c, end.x(), end.y());
        control = QPointF(end.x(), end.y() - c);
    } else {
        const QPointF mid((start.x() + end.x()) / 2.0, (start.y() + end.y()) / 2.0);
        path.moveTo(start);
        path.cubicTo(mid.x(), start.y(), mid.x(), end.y(), end.x(), end.y());
        control = mid;
    }

    QPen pen(color, selected ? 2.5 : 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);

    const double dx = end.x() - control.x();
    const double dy = end.y() - control.y();
    const double len = std::sqrt(dx * dx + dy * dy);
    if (len < 0.001) return;
    const double ux = dx / len;
    const double uy = dy / len;
    const double px = -uy;
    const double py = ux;

    const QPointF tip(end.x() + ux * PORT_R, end.y() + uy * PORT_R);
    const QPointF base(tip.x() - ux * 9, tip.y() - uy * 9);
    QPolygonF arrow;
    arrow << tip << QPointF(base.x() + px * 4.5, base.y() + py * 4.5)
          << QPointF(base.x() - px * 4.5, base.y() - py * 4.5);
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawPolygon(arrow);

    if (!from->isVariableNode()) {
        const QString label = edgeLabel(edge);
        painter.setFont(edgeFont());
        const QFontMetrics fm = painter.fontMetrics();
        const QPointF mid((start.x() + end.x()) / 2.0, (start.y() + end.y()) / 2.0);
        const int tw = fm.horizontalAdvance(label);
        const int th = fm.height();
        const QRectF bgRect(mid.x() - tw / 2.0 - 3, mid.y() - th / 2.0 - 1, tw + 6, th + 2);
        painter.setBrush(palette().base());
        painter.setPen(Qt::NoPen);
        painter.drawRect(bgRect);
        painter.setPen(color);
        painter.drawText(bgRect, Qt::AlignCenter, label);
    }
}

void FlowCanvas::drawPort(QPainter& painter, const QPointF& p, const QColor& color) {
    painter.setBrush(color);
    painter.setPen(QPen(palette().mid(), 1));
    painter.drawEllipse(p, PORT_R, PORT_R);
}

void FlowCanvas::drawVariableRows(QPainter& painter, const FlowNode& node) {
    const int w = nodeWidth(node);
    painter.save();
    painter.setClipRect(QRectF(node.x, node.y + HEADER_H, w, nodeHeight(node) - HEADER_H));

    painter.setFont(variableValueFont());
    QFontMetrics vfm(painter.font());
    const QColor valueColor = palette().color(QPalette::Disabled, QPalette::WindowText);

    if (node.vars.empty()) {
        painter.setPen(valueColor);
        painter.drawText(QPointF(node.x + 10, node.y + HEADER_H + 8 + vfm.ascent()),
                         QStringLiteral("(nessuna variabile)"));
        painter.restore();
        return;
    }

    const int count = static_cast<int>(node.vars.size());
    const int rows = std::min(count, VAR_MAX_ROWS);

    for (int i = 0; i < rows; ++i) {
        const auto& kv = node.vars[i];
        const double rowTop = node.y + HEADER_H + 6 + i * VAR_ROW_H;

        painter.setFont(variableNameFont());
        QFontMetrics nfm(painter.font());
        const QString name = QStringLiteral("${") + kv.first + QStringLiteral("}");
        painter.setPen(palette().windowText().color());
        painter.drawText(QPointF(node.x + 10, rowTop + nfm.ascent()), name);

        painter.setFont(variableValueFont());
        const int nameWidth = nfm.horizontalAdvance(name);
        const int valueX = node.x + 10 + nameWidth + 8;
        const int maxValueWidth = node.x + w - 10 - valueX;
        const QString value = fit(kv.second, vfm, maxValueWidth);
        painter.setPen(valueColor);
        painter.drawText(QPointF(valueX, rowTop + vfm.ascent()), value);
    }

    if (count > VAR_MAX_ROWS) {
        const double rowTop = node.y + HEADER_H + 6 + VAR_MAX_ROWS * VAR_ROW_H;
        painter.setFont(variableValueFont());
        painter.setPen(valueColor);
        painter.drawText(QPointF(node.x + 10, rowTop + vfm.ascent()),
                         QStringLiteral("+") + QString::number(count - VAR_MAX_ROWS) +
                             QStringLiteral(" altre variabili"));
    }

    painter.restore();
}

void FlowCanvas::drawRestBody(QPainter& painter, const FlowNode& node) {
    const int w = nodeWidth(node);
    const QString method = [&]() {
        if (node.call.is_object()) {
            auto m = node.call.find("method");
            if (m != node.call.end() && m->is_string()) return s2q(m->get<std::string>());
        }
        return QStringLiteral("GET");
    }();

    const QColor badgeColor = node.isSoapNode() ? COLOR_SOAP : methodColor(method);

    painter.save();
    painter.setClipRect(QRectF(node.x, node.y + HEADER_H, w, nodeHeight(node) - HEADER_H));

    painter.setFont(badgeFont());
    QFontMetrics bfm(painter.font());
    const QString badgeText = node.isSoapNode() ? QStringLiteral("SOAP") : method;
    const int badgeW = bfm.horizontalAdvance(badgeText) + 12;
    const int badgeH = bfm.height() + 2;
    const QRectF badgeRect(node.x + 8, node.y + HEADER_H + 8, badgeW, badgeH);
    painter.setPen(Qt::NoPen);
    painter.setBrush(badgeColor);
    painter.drawRoundedRect(badgeRect, 6, 6);
    painter.setPen(Qt::white);
    painter.drawText(badgeRect.adjusted(6, 0, -6, 0), Qt::AlignVCenter | Qt::AlignLeft, badgeText);

    auto resultIt = results_.find(node.id);
    QString status;
    QColor statusColor = palette().windowText().color();
    if (resultIt != results_.end()) {
        if (!resultIt->second.errorMessage.isEmpty()) {
            status = QStringLiteral("ERR");
            statusColor = COLOR_ERROR;
        } else {
            status = QString::number(resultIt->second.statusCode) + QStringLiteral(" \u00b7 ") +
                     QString::number(resultIt->second.latencyMs) + QStringLiteral("ms");
        }
    }

    QString url;
    if (node.call.is_object()) {
        auto u = node.call.find("url");
        if (u != node.call.end() && u->is_string()) url = s2q(u->get<std::string>());
    }

    painter.setFont(urlFont());
    QFontMetrics ufm(painter.font());
    int urlX = node.x + 8 + badgeW + 6;
    int statusW = status.isEmpty() ? 0 : ufm.horizontalAdvance(status);
    int maxUrlWidth = node.x + w - 8 - urlX - (status.isEmpty() ? 0 : statusW + 6);
    const QString urlText = fit(url, ufm, std::max(0, maxUrlWidth));
    painter.setPen(palette().windowText().color());
    painter.drawText(QPointF(urlX, node.y + HEADER_H + 8 + ufm.ascent()), urlText);

    if (!status.isEmpty()) {
        painter.setPen(statusColor);
        painter.drawText(QPointF(node.x + w - 8 - statusW, node.y + HEADER_H + 8 + ufm.ascent()),
                         status);
    }

    painter.restore();
}

void FlowCanvas::drawNode(QPainter& painter, const FlowNode& node) {
    const int w = nodeWidth(node);
    const int h = nodeHeight(node);

    const QColor badgeColor = node.isVariableNode() ? COLOR_VARIABLES
                              : (node.isSoapNode() ? COLOR_SOAP
                                                   : methodColor([&]() {
                                                         if (node.call.is_object()) {
                                                             auto m = node.call.find("method");
                                                             if (m != node.call.end() && m->is_string())
                                                                 return s2q(m->get<std::string>());
                                                         }
                                                         return QStringLiteral("GET");
                                                     }()));

    painter.setPen(Qt::NoPen);
    painter.setBrush(palette().base());
    painter.drawRoundedRect(node.x, node.y, w, h, 10, 10);

    painter.save();
    painter.setClipRect(QRectF(node.x, node.y, w, HEADER_H));
    painter.setBrush(badgeColor);
    painter.drawRoundedRect(node.x, node.y, w, HEADER_H * 2, 10, 10);
    painter.restore();

    painter.setFont(methodFont());
    painter.setPen(Qt::white);
    QFontMetrics tfm(painter.font());
    QString title = node.title;
    if (title.isEmpty()) {
        title = node.isVariableNode() ? QStringLiteral("VARIABILI")
               : (node.isSoapNode() ? QStringLiteral("SOAP") : QStringLiteral("GET"));
    }
    const QString clippedTitle = fit(title, tfm, w - 32);
    painter.drawText(QRectF(node.x + 24, node.y, w - 32, HEADER_H), Qt::AlignVCenter | Qt::AlignLeft,
                     clippedTitle);

    painter.save();
    if (node.isVariableNode()) {
        painter.setFont(variableNameFont());
        QFontMetrics dfm(painter.font());
        const QRectF chipRect(node.x + 4, node.y + (HEADER_H - 20) / 2.0, 20, 20);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 255, 255, 220));
        painter.drawRoundedRect(chipRect, 6, 6);
        painter.setPen(COLOR_VARIABLES);
        painter.drawText(chipRect, Qt::AlignCenter, QStringLiteral("$"));
    } else {
        const QRectF iconRect(node.x + 6, node.y + (HEADER_H - 16) / 2.0, 16, 16);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 255, 255, 220));
        painter.drawRoundedRect(iconRect, 6, 6);
        drawIcon(painter, iconRect, node.isSoapNode());
    }
    painter.restore();

    if (node.isVariableNode()) {
        drawVariableRows(painter, node);
    } else {
        drawRestBody(painter, node);
    }

    const bool selected = selectedNodeId_ == node.id;
    auto resultIt = results_.find(node.id);
    painter.setBrush(Qt::NoBrush);
    if (selected) {
        painter.setPen(QPen(palette().highlight(), 2.2));
    } else if (resultIt != results_.end()) {
        const bool ok = resultIt->second.errorMessage.isEmpty() && resultIt->second.success;
        painter.setPen(QPen(ok ? COLOR_SUCCESS : COLOR_ERROR, 2.0));
    } else {
        painter.setPen(QPen(palette().mid(), 1.2));
    }
    painter.drawRoundedRect(node.x, node.y, w, h, 10, 10);

    if (node.isVariableNode()) {
        drawPort(painter, outputPoint(node), badgeColor);
    } else {
        drawPort(painter, inputPoint(node), badgeColor);
        drawPort(painter, outputPoint(node), badgeColor);
    }
}

void FlowCanvas::drawRubberBand(QPainter& painter) {
    const FlowNode* from = graph_.nodeById(connectingFromId_);
    if (!from) return;
    const QPointF start = outputPoint(*from);
    painter.setPen(QPen(palette().windowText().color(), 1.6, Qt::DashLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawLine(start, connectCursor_);
}

void FlowCanvas::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    painter.translate(pan_);
    painter.scale(zoom_, zoom_);

    for (const auto& group : graph_.groups()) drawGroup(painter, group);
    for (const auto& edge : graph_.edges()) drawEdge(painter, edge);
    for (const auto& node : graph_.nodes()) drawNode(painter, node);

    if (dragMode_ == CONNECT) drawRubberBand(painter);
}

void FlowCanvas::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::RightButton) {
        return;
    }
    const QPointF flow = toFlow(event->pos());
    setFocus();

    if (const FlowNode* portNode = portAt(flow, true)) {
        connectingFromId_ = portNode->id;
        connectCursor_ = flow;
        selectNode(portNode->id);
        dragMode_ = CONNECT;
        return;
    }

    if (const FlowNode* node = nodeAt(flow)) {
        selectNode(node->id);
        dragMode_ = NODE;
        dragStartMouse_ = event->pos();
        dragStartNodePos_ = QPoint(node->x, node->y);
        hoverGroupId_.clear();
        return;
    }

    if (const FlowGroup* corner = groupResizeCornerAt(flow)) {
        selectGroup(corner->id);
        resizingGroupId_ = corner->id;
        dragMode_ = RESIZE;
        update();
        return;
    }

    if (const FlowGroup* header = groupHeaderAt(flow)) {
        selectGroup(header->id);
        dragStartMouse_ = event->pos();
        dragStartGroupPos_ = QPoint(header->x, header->y);
        dragMode_ = GROUP;
        update();
        return;
    }

    if (const FlowEdge* edge = edgeAt(flow)) {
        selectedEdgeId_ = edge->id;
        selectedNodeId_.clear();
        selectedGroupId_.clear();
        update();
        return;
    }

    selectedNodeId_.clear();
    selectedEdgeId_.clear();
    selectedGroupId_.clear();
    dragStartMouse_ = event->pos();
    dragStartPan_ = pan_;
    dragMode_ = PAN;
    setCursor(Qt::ClosedHandCursor);
    update();
    if (listener_.selectionChanged) listener_.selectionChanged();
}

void FlowCanvas::mouseMoveEvent(QMouseEvent* event) {
    const QPointF flow = toFlow(event->pos());

    if (dragMode_ == NODE) {
        const FlowNode* node = graph_.nodeById(selectedNodeId_);
        if (!node) return;
        const double z = safeZoom();
        const double dx = (event->pos().x() - dragStartMouse_.x()) / z;
        const double dy = (event->pos().y() - dragStartMouse_.y()) / z;
        const int nx = clampCoord(snap(static_cast<int>(dragStartNodePos_.x() + dx)));
        const int ny = clampCoord(snap(static_cast<int>(dragStartNodePos_.y() + dy)));
        graph_.moveNode(node->id, nx, ny);
        const FlowNode* moved = graph_.nodeById(node->id);
        const FlowGroup* g = moved ? groupContainingNode(*moved) : nullptr;
        hoverGroupId_ = g ? g->id : QString();
        update();
    } else if (dragMode_ == GROUP) {
        const FlowGroup* g = graph_.groupById(selectedGroupId_);
        if (!g) return;
        const double z = safeZoom();
        const double dx = (event->pos().x() - dragStartMouse_.x()) / z;
        const double dy = (event->pos().y() - dragStartMouse_.y()) / z;
        const int nx = clampCoord(snap(static_cast<int>(dragStartGroupPos_.x() + dx)));
        const int ny = clampCoord(snap(static_cast<int>(dragStartGroupPos_.y() + dy)));
        graph_.moveGroup(g->id, nx - g->x, ny - g->y);
        update();
    } else if (dragMode_ == RESIZE) {
        const FlowGroup* g = graph_.groupById(resizingGroupId_);
        if (!g) return;
        const int nw = std::max(GROUP_MIN_W, clampCoord(snap(static_cast<int>(flow.x())) - g->x));
        const int nh = std::max(GROUP_MIN_H, clampCoord(snap(static_cast<int>(flow.y())) - g->y));
        graph_.resizeGroup(g->id, g->x, g->y, nw, nh);
        update();
    } else if (dragMode_ == CONNECT) {
        connectCursor_ = flow;
        update();
    } else if (dragMode_ == PAN) {
        pan_ = QPointF(dragStartPan_.x() + (event->pos().x() - dragStartMouse_.x()),
                       dragStartPan_.y() + (event->pos().y() - dragStartMouse_.y()));
        update();
    }
}

void FlowCanvas::mouseReleaseEvent(QMouseEvent* event) {
    if (dragMode_ == NODE) {
        const FlowNode* node = graph_.nodeById(selectedNodeId_);
        if (node) {
            const FlowGroup* g = groupContainingNode(*node);
            graph_.setNodeGroup(node->id, g ? g->id : QString());
        }
        hoverGroupId_.clear();
        dragMode_ = NONE;
        if (listener_.graphChanged) listener_.graphChanged();
        update();
        return;
    }
    if (dragMode_ == GROUP) {
        dragMode_ = NONE;
        if (listener_.graphChanged) listener_.graphChanged();
        update();
        return;
    }
    if (dragMode_ == RESIZE) {
        dragMode_ = NONE;
        if (listener_.graphChanged) listener_.graphChanged();
        update();
        return;
    }
    if (dragMode_ == CONNECT) {
        const QPointF flow = toFlow(event->pos());
        const FlowNode* target = portAt(flow, false);
        if (!target) target = nodeAt(flow);
        if (target && target->id != connectingFromId_) {
            if (listener_.createEdgeRequested) {
                listener_.createEdgeRequested(connectingFromId_, target->id);
            }
        }
        dragMode_ = NONE;
        connectingFromId_.clear();
        update();
        return;
    }
    if (dragMode_ == PAN) {
        dragMode_ = NONE;
        setCursor(Qt::ArrowCursor);
        update();
        return;
    }
}

void FlowCanvas::mouseDoubleClickEvent(QMouseEvent* event) {
    const QPointF flow = toFlow(event->pos());
    if (const FlowNode* node = nodeAt(flow)) {
        if (listener_.editNode) listener_.editNode(*node);
        return;
    }
    if (const FlowEdge* edge = edgeAt(flow)) {
        if (listener_.editEdge) listener_.editEdge(*edge);
        return;
    }
}

void FlowCanvas::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        const double factor = event->angleDelta().y() > 0 ? 1.1 : 0.9;
        const QPointF flow = toFlow(event->position().toPoint());
        const double newZoom = std::max(0.3, std::min(3.0, zoom_ * factor));
        pan_ = QPointF(event->position().x() - flow.x() * newZoom,
                       event->position().y() - flow.y() * newZoom);
        zoom_ = newZoom;
        update();
    } else {
        const int units = event->angleDelta().y() / 8;
        pan_.ry() -= units * 10;
        update();
    }
    event->accept();
}

void FlowCanvas::duplicateNode(const FlowNode& node) {
    QString newId = node.id + QStringLiteral("_copy");
    int suffix = 2;
    while (graph_.nodeById(newId)) {
        newId = node.id + QStringLiteral("_copy") + QString::number(suffix++);
    }
    FlowNode copy = node.moveTo(node.x + 30, node.y + 30);
    copy.id = newId;
    graph_.nodes().push_back(copy);
    selectNode(newId);
    if (listener_.graphChanged) listener_.graphChanged();
}

void FlowCanvas::duplicateSelectedNode() {
    const FlowNode* node = graph_.nodeById(selectedNodeId_);
    if (node) duplicateNode(*node);
}

void FlowCanvas::deleteNode(const QString& id) {
    graph_.removeNode(id);
    if (listener_.graphChanged) listener_.graphChanged();
}

void FlowCanvas::deleteEdge(const QString& id) {
    graph_.removeEdge(id);
    if (listener_.graphChanged) listener_.graphChanged();
}

void FlowCanvas::deleteGroup(const QString& id) {
    graph_.removeGroup(id);
    if (listener_.graphChanged) listener_.graphChanged();
}

void FlowCanvas::deleteSelection() {
    if (!selectedEdgeId_.isEmpty()) {
        deleteEdge(selectedEdgeId_);
        return;
    }
    if (!selectedNodeId_.isEmpty()) {
        deleteNode(selectedNodeId_);
        return;
    }
    if (!selectedGroupId_.isEmpty()) {
        deleteGroup(selectedGroupId_);
        return;
    }
}

void FlowCanvas::addGroup(const QString& name) {
    int maxId = 0;
    for (const auto& g : graph_.groups()) {
        const QString id = g.id;
        if (id.startsWith(QLatin1Char('g'))) {
            bool ok = false;
            const int n = id.mid(1).toInt(&ok);
            if (ok) maxId = std::max(maxId, n);
        }
    }
    const QString id = QStringLiteral("g") + QString::number(maxId + 1);

    QPointF center = toFlow(QPoint(width() / 2, height() / 2));
    const int x = snap(static_cast<int>(center.x() - 170));
    const int y = snap(static_cast<int>(center.y() - 120));

    graph_.addGroup(FlowGroup(id, name, x, y, 340, 240, {}, {}));
    selectedGroupId_ = id;
    selectedNodeId_.clear();
    selectedEdgeId_.clear();
    if (listener_.graphChanged) listener_.graphChanged();
}

void FlowCanvas::fitView() {
    if (graph_.nodes().empty() && graph_.groups().empty()) {
        zoom_ = 1.0;
        pan_ = QPointF(60, 40);
        update();
        return;
    }
    double minX = 1e9, minY = 1e9, maxX = -1e9, maxY = -1e9;
    bool found = false;
    for (const auto& n : graph_.nodes()) {
        minX = std::min(minX, static_cast<double>(n.x));
        minY = std::min(minY, static_cast<double>(n.y));
        maxX = std::max(maxX, static_cast<double>(n.x + nodeWidth(n)));
        maxY = std::max(maxY, static_cast<double>(n.y + nodeHeight(n)));
        found = true;
    }
    for (const auto& g : graph_.groups()) {
        minX = std::min(minX, static_cast<double>(g.x));
        minY = std::min(minY, static_cast<double>(g.y));
        maxX = std::max(maxX, static_cast<double>(g.x + g.w));
        maxY = std::max(maxY, static_cast<double>(g.y + g.h));
        found = true;
    }
    if (!found) return;
    const double contentW = maxX - minX;
    const double contentH = maxY - minY;
    const double zx = (width() - 80.0) / std::max(1.0, contentW);
    const double zy = (height() - 80.0) / std::max(1.0, contentH);
    zoom_ = std::max(0.3, std::min(2.0, std::min(zx, zy)));
    pan_ = QPointF(40 - minX * zoom_, 40 - minY * zoom_);
    update();
}

void FlowCanvas::keyPressEvent(QKeyEvent* event) {
    if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_D) {
        duplicateSelectedNode();
        return;
    }
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        deleteSelection();
        return;
    }
    QWidget::keyPressEvent(event);
}

void FlowCanvas::buildNodeMenu(QMenu& menu, const FlowNode& node) {
    QAction* edit = menu.addAction(QStringLiteral("Modifica..."));
    QAction* runFrom = menu.addAction(QStringLiteral("Esegui da qui"));
    menu.addSeparator();
    QAction* duplicate = menu.addAction(QStringLiteral("Duplica"));
    QAction* remove = menu.addAction(QStringLiteral("Elimina"));

    connect(edit, &QAction::triggered, this, [this, node]() {
        if (listener_.editNode) listener_.editNode(node);
    });
    connect(runFrom, &QAction::triggered, this, [this, node]() {
        if (listener_.runFromNodeRequested) listener_.runFromNodeRequested(node.id);
    });
    connect(duplicate, &QAction::triggered, this, [this, node]() { duplicateNode(node); });
    connect(remove, &QAction::triggered, this, [this, node]() { deleteNode(node.id); });
}

void FlowCanvas::buildEdgeMenu(QMenu& menu, const FlowEdge& edge) {
    QAction* edit = menu.addAction(QStringLiteral("Modifica link..."));
    menu.addSeparator();
    QAction* remove = menu.addAction(QStringLiteral("Elimina link"));
    connect(edit, &QAction::triggered, this, [this, edge]() {
        if (listener_.editEdge) listener_.editEdge(edge);
    });
    connect(remove, &QAction::triggered, this, [this, edge]() { deleteEdge(edge.id); });
}

void FlowCanvas::buildGroupMenu(QMenu& menu, const FlowGroup& group) {
    QAction* vars = menu.addAction(QStringLiteral("Variabili collection..."));
    QAction* rename = menu.addAction(QStringLiteral("Rinomina collection..."));
    menu.addSeparator();
    QAction* remove = menu.addAction(QStringLiteral("Elimina collection"));
    connect(vars, &QAction::triggered, this, [this, group]() {
        if (listener_.editGroupVariablesRequested) listener_.editGroupVariablesRequested(group);
    });
    connect(rename, &QAction::triggered, this, [this, group]() {
        if (listener_.renameGroupRequested) listener_.renameGroupRequested(group);
    });
    connect(remove, &QAction::triggered, this, [this, group]() { deleteGroup(group.id); });
}

void FlowCanvas::buildEmptyMenu(QMenu& menu) {
    QMenu* newMenu = menu.addMenu(QStringLiteral("Nuovo"));
    QAction* restAction = newMenu->addAction(QStringLiteral("Chiamata REST..."));
    QAction* soapAction = newMenu->addAction(QStringLiteral("Chiamata SOAP..."));
    QAction* varAction = newMenu->addAction(QStringLiteral("Nodo variabili..."));
    QAction* groupAction = menu.addAction(QStringLiteral("Nuova collection..."));
    menu.addSeparator();
    QAction* runAllAction = menu.addAction(QStringLiteral("Esegui tutto"));
    menu.addSeparator();
    QAction* globalVarsAction = menu.addAction(QStringLiteral("Variabili globali..."));
    QAction* fitAction = menu.addAction(QStringLiteral("Adatta alla vista"));

    connect(restAction, &QAction::triggered, this, [this]() {
        if (listener_.addNodeRequested) listener_.addNodeRequested();
    });
    connect(soapAction, &QAction::triggered, this, [this]() {
        if (listener_.addSoapNodeRequested) listener_.addSoapNodeRequested();
    });
    connect(varAction, &QAction::triggered, this, [this]() {
        if (listener_.addVariableNodeRequested) listener_.addVariableNodeRequested();
    });
    connect(groupAction, &QAction::triggered, this, [this]() {
        if (listener_.addGroupRequested) listener_.addGroupRequested();
    });
    connect(runAllAction, &QAction::triggered, this, [this]() {
        if (listener_.runAllRequested) listener_.runAllRequested();
    });
    connect(globalVarsAction, &QAction::triggered, this, [this]() {
        if (listener_.editGlobalVariablesRequested) listener_.editGlobalVariablesRequested();
    });
    connect(fitAction, &QAction::triggered, this, [this]() { fitView(); });
}

void FlowCanvas::contextMenuEvent(QContextMenuEvent* event) {
    const QPointF flow = toFlow(event->pos());

    QMenu menu(this);
    if (const FlowNode* node = nodeAt(flow)) {
        selectNode(node->id);
        update();
        buildNodeMenu(menu, *node);
    } else if (const FlowEdge* edge = edgeAt(flow)) {
        selectedEdgeId_ = edge->id;
        selectedNodeId_.clear();
        selectedGroupId_.clear();
        update();
        buildEdgeMenu(menu, *edge);
    } else if (const FlowGroup* group = groupHeaderAt(flow)) {
        selectGroup(group->id);
        buildGroupMenu(menu, *group);
    } else {
        buildEmptyMenu(menu);
    }

    if (!menu.actions().isEmpty()) {
        menu.exec(event->globalPos());
    }
}

} // namespace rcm
