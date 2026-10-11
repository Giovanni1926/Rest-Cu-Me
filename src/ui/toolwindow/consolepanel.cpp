#include "consolepanel.h"

#include <QVBoxLayout>
#include <QFontDatabase>
#include <QScrollBar>
#include <QPalette>
#include <QApplication>

namespace rcm {

ConsolePanel::ConsolePanel(QWidget* parent) : QWidget(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    textEdit_ = new QTextEdit(this);
    textEdit_->setReadOnly(true);
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(10);
    textEdit_->setFont(font);
    textEdit_->document()->setDefaultStyleSheet(QStringLiteral("body { white-space: pre-wrap; }"));
    layout->addWidget(textEdit_);
}

bool ConsolePanel::isDarkTheme() {
    const QPalette palette = QApplication::palette();
    return palette.color(QPalette::Window).lightness() < 128;
}

void ConsolePanel::refresh(const std::vector<ConsoleLine>& log) {
    textEdit_->clear();

    const bool dark = isDarkTheme();
    auto colorFor = [&](ConsoleLine::Kind kind) {
        switch (kind) {
        case ConsoleLine::SUCCESS: return dark ? QStringLiteral("#81C784") : QStringLiteral("#1B5E20");
        case ConsoleLine::ERROR: return dark ? QStringLiteral("#EF5350") : QStringLiteral("#B71C1C");
        case ConsoleLine::HEADER: return dark ? QStringLiteral("#64B5F6") : QStringLiteral("#0D47A1");
        case ConsoleLine::MUTED: return dark ? QStringLiteral("#9E9E9E") : QStringLiteral("#616161");
        case ConsoleLine::INFO: default: return QString();
        }
    };
    auto prefixFor = [&](ConsoleLine::Kind kind) {
        switch (kind) {
        case ConsoleLine::SUCCESS: return QStringLiteral("\u2713  ");
        case ConsoleLine::ERROR: return QStringLiteral("\u2717  ");
        case ConsoleLine::HEADER: return QStringLiteral("\u25b6  ");
        case ConsoleLine::MUTED: return QStringLiteral("\u2022  ");
        case ConsoleLine::INFO: default: return QStringLiteral("   ");
        }
    };

    QString html = QStringLiteral("<pre>");
    for (const auto& line : log) {
        const QString color = colorFor(line.kind);
        const bool bold = line.kind == ConsoleLine::ERROR || line.kind == ConsoleLine::HEADER;
        QString style;
        if (!color.isEmpty()) style += QStringLiteral("color:%1;").arg(color);
        if (bold) style += QStringLiteral("font-weight:bold;");
        html += QStringLiteral("<span style=\"%1\">%2%3</span><br/>")
                    .arg(style, prefixFor(line.kind).toHtmlEscaped(), line.text.toHtmlEscaped());
    }
    html += QStringLiteral("</pre>");
    textEdit_->setHtml(html);
    textEdit_->verticalScrollBar()->setValue(textEdit_->verticalScrollBar()->maximum());
}

} // namespace rcm
