#pragma once

#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include <QColor>

namespace rcm {

inline QIcon globeIcon(const QColor& color) {
    QPixmap pm(16, 16);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(color, 1.3);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    const QRectF circle(2.5, 2.5, 11, 11);
    p.drawEllipse(circle);
    p.drawEllipse(QRectF(6, 2.5, 4, 11));
    p.drawLine(QPointF(2.5, 8), QPointF(13.5, 8));
    return QIcon(pm);
}

inline QIcon historyIcon(const QColor& color) {
    QPixmap pm(16, 16);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(color, 1.3);
    pen.setJoinStyle(Qt::RoundJoin);
    pen.setCapStyle(Qt::RoundCap);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(QRectF(2.5, 2.5, 11, 11));
    p.drawLine(QPointF(8, 8), QPointF(8, 4.5));
    p.drawLine(QPointF(8, 8), QPointF(11, 9.5));
    p.drawLine(QPointF(8, 2), QPointF(8, 3.5));
    return QIcon(pm);
}

inline QIcon soapIcon(const QColor& color) {
    QPixmap pm(16, 16);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(color);
    p.drawRoundedRect(QRectF(2, 2, 12, 12), 3, 3);
    p.setBrush(QColor(255, 255, 255));
    p.drawEllipse(QPointF(5.5, 6), 1.5, 1.5);
    p.drawEllipse(QPointF(10, 9), 2, 2);
    p.drawEllipse(QPointF(12, 4.5), 1.2, 1.2);
    return QIcon(pm);
}

inline QIcon dollarIcon(const QColor& color) {
    QPixmap pm(16, 16);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    QFont f = p.font();
    f.setBold(true);
    f.setPointSize(10);
    p.setFont(f);
    p.setPen(color);
    p.drawText(QRectF(0, 0, 16, 16), Qt::AlignCenter, QStringLiteral("$"));
    return QIcon(pm);
}

inline QIcon collectionIcon(const QColor& color) {
    QPixmap pm(16, 16);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(color, 1.3);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(QRectF(2, 3, 12, 10), 2, 2);
    p.drawLine(QPointF(2, 7), QPointF(14, 7));
    return QIcon(pm);
}

inline QIcon graphIcon(const QColor& color) {
    QPixmap pm(16, 16);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(color, 1.3);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    p.drawLine(QPointF(3, 5), QPointF(7, 11));
    p.drawLine(QPointF(7, 11), QPointF(13, 6));
    p.drawLine(QPointF(3, 5), QPointF(13, 6));
    p.setBrush(color);
    p.drawEllipse(QPointF(3, 5), 2, 2);
    p.drawEllipse(QPointF(7, 11), 2, 2);
    p.drawEllipse(QPointF(13, 6), 2, 2);
    return QIcon(pm);
}

inline QIcon visualIcon(const QColor& color) {
    QPixmap pm(16, 16);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(color, 1.3);
    pen.setJoinStyle(Qt::RoundJoin);
    pen.setCapStyle(Qt::RoundCap);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(QRectF(2, 2, 12, 12), 1.5, 1.5);
    p.drawLine(QPointF(5, 5.5), QPointF(11, 5.5));
    p.drawLine(QPointF(5, 8), QPointF(11, 8));
    p.drawLine(QPointF(5, 10.5), QPointF(9, 10.5));
    return QIcon(pm);
}

} // namespace rcm
