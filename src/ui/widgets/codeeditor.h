#pragma once

#include <QPlainTextEdit>
#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QFontDatabase>
#include <QTextCharFormat>

namespace rcm {

class JsonHighlighter : public QSyntaxHighlighter {
public:
    explicit JsonHighlighter(QTextDocument* doc) : QSyntaxHighlighter(doc) {
        keyFormat_.setForeground(QColor(0x1E, 0x88, 0xE5));
        keyFormat_.setFontWeight(QFont::Bold);
        stringFormat_.setForeground(QColor(0x2E, 0x7D, 0x32));
        numberFormat_.setForeground(QColor(0xEF, 0x6C, 0x00));
        boolFormat_.setForeground(QColor(0x8E, 0x24, 0xAA));
    }

protected:
    void highlightBlock(const QString& text) override {
        QRegularExpression keyRe(QStringLiteral("\"(\\\\.|[^\"\\\\])*\"(\\s*:)"));
        QRegularExpression stringRe(QStringLiteral("\"(\\\\.|[^\"\\\\])*\""));
        QRegularExpression numberRe(QStringLiteral("\\b-?\\d+(\\.\\d+)?([eE][+-]?\\d+)?\\b"));
        QRegularExpression boolRe(QStringLiteral("\\b(true|false|null)\\b"));

        auto apply = [&](const QRegularExpression& re, const QTextCharFormat& fmt) {
            auto it = re.globalMatch(text);
            while (it.hasNext()) {
                const auto m = it.next();
                setFormat(m.capturedStart(), m.capturedLength(), fmt);
            }
        };
        apply(keyRe, keyFormat_);
        apply(stringRe, stringFormat_);
        apply(numberRe, numberFormat_);
        apply(boolRe, boolFormat_);
    }

private:
    QTextCharFormat keyFormat_;
    QTextCharFormat stringFormat_;
    QTextCharFormat numberFormat_;
    QTextCharFormat boolFormat_;
};

class XmlHighlighter : public QSyntaxHighlighter {
public:
    explicit XmlHighlighter(QTextDocument* doc) : QSyntaxHighlighter(doc) {
        tagFormat_.setForeground(QColor(0x1E, 0x88, 0xE5));
        tagFormat_.setFontWeight(QFont::Bold);
        attrFormat_.setForeground(QColor(0x8E, 0x24, 0xAA));
        commentFormat_.setForeground(QColor(0x75, 0x75, 0x75));
        stringFormat_.setForeground(QColor(0x2E, 0x7D, 0x32));
    }

protected:
    void highlightBlock(const QString& text) override {
        QRegularExpression commentRe(QStringLiteral("<!--.*-->"));
        auto it = commentRe.globalMatch(text);
        while (it.hasNext()) {
            const auto m = it.next();
            setFormat(m.capturedStart(), m.capturedLength(), commentFormat_);
        }
        QRegularExpression tagRe(QStringLiteral("<[^!?][^>]*>"));
        auto tagIt = tagRe.globalMatch(text);
        while (tagIt.hasNext()) {
            const auto m = tagIt.next();
            setFormat(m.capturedStart(), m.capturedLength(), tagFormat_);
        }
        QRegularExpression attrRe(QStringLiteral("\\s[a-zA-Z_:][-a-zA-Z0-9_.:]*\\s*="));
        auto attrIt = attrRe.globalMatch(text);
        while (attrIt.hasNext()) {
            const auto m = attrIt.next();
            setFormat(m.capturedStart(), m.capturedLength(), attrFormat_);
        }
        QRegularExpression stringRe(QStringLiteral("\"[^\"]*\""));
        auto strIt = stringRe.globalMatch(text);
        while (strIt.hasNext()) {
            const auto m = strIt.next();
            setFormat(m.capturedStart(), m.capturedLength(), stringFormat_);
        }
    }

private:
    QTextCharFormat tagFormat_;
    QTextCharFormat attrFormat_;
    QTextCharFormat commentFormat_;
    QTextCharFormat stringFormat_;
};

class CodeEditor : public QPlainTextEdit {
public:
    enum Mode { Plain, Json, Xml };

    explicit CodeEditor(QWidget* parent = nullptr) : QPlainTextEdit(parent) {
        QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        font.setPointSize(11);
        setFont(font);
    }

    void setMode(Mode mode) {
        mode_ = mode;
        switch (mode) {
        case Json: highlighter_ = std::make_unique<JsonHighlighter>(document()); break;
        case Xml: highlighter_ = std::make_unique<XmlHighlighter>(document()); break;
        case Plain: default: highlighter_.reset(); break;
        }
    }

    Mode mode() const { return mode_; }

private:
    Mode mode_ = Plain;
    std::unique_ptr<QSyntaxHighlighter> highlighter_;
};

} // namespace rcm
