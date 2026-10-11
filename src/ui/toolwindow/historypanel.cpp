#include "historypanel.h"
#include "../../run/rcmrunservice.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QScrollArea>
#include <QTreeWidget>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QFontDatabase>
#include <QFrame>

namespace rcm {

static bool isDarkTheme() {
    return QApplication::palette().color(QPalette::Window).lightness() < 128;
}

static QColor okColor() { return isDarkTheme() ? QColor(0x66, 0xBB, 0x6A) : QColor(0x2E, 0x7D, 0x32); }
static QColor koColor() { return isDarkTheme() ? QColor(0xEF, 0x53, 0x50) : QColor(0xC6, 0x28, 0x28); }
static QColor okBg() { return isDarkTheme() ? QColor(0x22, 0x33, 0x1F) : QColor(0xE8, 0xF5, 0xE9); }
static QColor koBg() { return isDarkTheme() ? QColor(0x3A, 0x20, 0x20) : QColor(0xFF, 0xEB, 0xEE); }
static QColor subtle() { return isDarkTheme() ? QColor(0x9E, 0x9E, 0x9E) : QColor(0x61, 0x61, 0x61); }

static bool isJson(const QString& content) {
    try {
        Json::parse(q2s(content));
        return true;
    } catch (...) {
        return false;
    }
}

static void buildJsonTree(QTreeWidget* tree, const QString& json) {
    tree->clear();
    tree->setHeaderHidden(true);
    try {
        Json root = Json::parse(q2s(json));
        std::function<void(QTreeWidgetItem*, const Json&)> build = [&](QTreeWidgetItem* parent,
                                                                       const Json& element) {
            if (element.is_object()) {
                for (auto it = element.begin(); it != element.end(); ++it) {
                    const QString key = s2q(it.key());
                    if (it.value().is_object() || it.value().is_array()) {
                        QTreeWidgetItem* item = new QTreeWidgetItem(parent, QStringList(key));
                        build(item, it.value());
                    } else {
                        new QTreeWidgetItem(parent,
                                            QStringList(key + QStringLiteral(": ") +
                                                        s2q(it.value().is_null()
                                                                ? std::string("null")
                                                                : it.value().dump())));
                    }
                }
            } else if (element.is_array()) {
                int i = 0;
                for (const auto& item : element) {
                    if (item.is_object() || item.is_array()) {
                        QTreeWidgetItem* node = new QTreeWidgetItem(
                            parent, QStringList(QStringLiteral("[") + QString::number(i) + QStringLiteral("]")));
                        build(node, item);
                    } else {
                        new QTreeWidgetItem(
                            parent, QStringList(QStringLiteral("[") + QString::number(i) +
                                               QStringLiteral("]: ") + s2q(item.dump())));
                    }
                    ++i;
                }
            }
        };
        QTreeWidgetItem* rootItem = new QTreeWidgetItem(tree, QStringList(QStringLiteral("Response")));
        build(rootItem, root);
        tree->setRootIsDecorated(true);
    } catch (...) {
        new QTreeWidgetItem(tree, QStringList(json));
    }
}

class RecordCard : public QFrame {
public:
    RecordCard(const CallRecord& record, QWidget* parent = nullptr) : QFrame(parent), record_(record) {
        setFrameShape(QFrame::NoFrame);

        QVBoxLayout* outer = new QVBoxLayout(this);
        outer->setContentsMargins(0, 0, 0, 0);
        outer->setSpacing(0);

        const QColor accent = record.isSuccess() ? okColor() : koColor();

        QWidget* accentBar = new QWidget(this);
        accentBar->setFixedWidth(3);
        accentBar->setStyleSheet(QStringLiteral("background-color: %1;").arg(accent.name()));

        QWidget* content = new QWidget(this);
        QVBoxLayout* contentLayout = new QVBoxLayout(content);
        contentLayout->setContentsMargins(8, 6, 8, 4);
        contentLayout->setSpacing(4);

        QWidget* headerBar = new QWidget(content);
        headerBar->setCursor(Qt::PointingHandCursor);
        headerBar->setAutoFillBackground(true);
        QPalette hp = headerBar->palette();
        hp.setColor(QPalette::Window, record.isSuccess() ? okBg() : koBg());
        headerBar->setPalette(hp);
        QHBoxLayout* headerLayout = new QHBoxLayout(headerBar);
        headerLayout->setContentsMargins(6, 4, 6, 4);

        QLabel* chip = new QLabel(headerBar);
        chip->setText(record.statusCode == 0 ? QStringLiteral("ERR") : QString::number(record.statusCode));
        chip->setAlignment(Qt::AlignCenter);
        chip->setStyleSheet(QStringLiteral("background-color: %1; color: white; font-weight: bold; padding: 3px 8px; border-radius: 3px;")
                                .arg(accent.name()));
        headerLayout->addWidget(chip);

        QString titleHtml;
        if (!record.label.isEmpty()) {
            titleHtml += QStringLiteral("<b>") + record.label.toHtmlEscaped() + QStringLiteral("</b>  ");
        }
        titleHtml += record.method.toHtmlEscaped() + QStringLiteral(" ") + record.url.toHtmlEscaped();
        titleHtml += QStringLiteral("  <font color='gray'>") + record.timestamp.toHtmlEscaped() + QStringLiteral("</font>");
        QLabel* title = new QLabel(titleHtml, headerBar);
        title->setTextFormat(Qt::RichText);
        headerLayout->addWidget(title, 1);

        QPushButton* replayButton = new QPushButton(QStringLiteral("Riesegui"), headerBar);
        headerLayout->addWidget(replayButton);
        connect(replayButton, &QPushButton::clicked, this, [this]() {
            RcmRunService::instance().replay(record_);
        });

        contentLayout->addWidget(headerBar);

        details_ = new QWidget(content);
        QVBoxLayout* detailsLayout = new QVBoxLayout(details_);
        detailsLayout->setContentsMargins(8, 8, 4, 4);
        detailsLayout->setSpacing(4);
        detailsLayout->addWidget(buildResponseSection());
        detailsLayout->addSpacing(8);
        detailsLayout->addWidget(buildRequestSection());
        details_->setVisible(false);
        contentLayout->addWidget(details_);

        QHBoxLayout* row = new QHBoxLayout();
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(0);
        row->addWidget(accentBar);
        row->addWidget(content, 1);
        outer->addLayout(row);

        auto toggle = [this]() {
            details_->setVisible(!details_->isVisible());
        };
        headerBar->installEventFilter(new ClickFilter(toggle, headerBar));
    }

private:
    class ClickFilter : public QObject {
    public:
        ClickFilter(std::function<void()> fn, QObject* parent) : QObject(parent), fn_(fn) {}
        bool eventFilter(QObject*, QEvent* event) override {
            if (event->type() == QEvent::MouseButtonRelease) {
                fn_();
                return true;
            }
            return false;
        }

    private:
        std::function<void()> fn_;
    };

    QWidget* buildResponseSection() {
        QWidget* section = new QWidget();
        QVBoxLayout* layout = new QVBoxLayout(section);
        layout->setContentsMargins(0, 0, 0, 0);

        QLabel* title = new QLabel(QStringLiteral("Risposta"), section);
        QFont tf = title->font();
        tf.setBold(true);
        title->setFont(tf);
        layout->addWidget(title);

        QLabel* statusLine = new QLabel(section);
        if (record_.statusCode == 0) {
            statusLine->setText(QStringLiteral("Errore / nessuna risposta"));
        } else {
            statusLine->setText(QString::number(record_.statusCode) +
                                (record_.isSuccess() ? QStringLiteral("  OK") : QStringLiteral("  errore")));
        }
        statusLine->setStyleSheet(QStringLiteral("color: %1; font-weight: bold;")
                                      .arg(record_.isSuccess() ? okColor().name() : koColor().name()));
        layout->addWidget(statusLine);
        layout->addSpacing(6);
        layout->addWidget(keyValue(QStringLiteral("Headers"), record_.responseHeaders));
        layout->addWidget(keyValue(QStringLiteral("Cookies"), record_.responseCookies));
        layout->addWidget(bodyWidget(QStringLiteral("Body"), record_.responseBody));
        return section;
    }

    QWidget* buildRequestSection() {
        QWidget* section = new QWidget();
        QVBoxLayout* layout = new QVBoxLayout(section);
        layout->setContentsMargins(0, 0, 0, 0);

        QLabel* title = new QLabel(record_.method + QStringLiteral(" ") + record_.url, section);
        QFont tf = title->font();
        tf.setBold(true);
        title->setFont(tf);
        layout->addWidget(title);
        layout->addSpacing(6);
        layout->addWidget(keyValue(QStringLiteral("Headers"), record_.requestHeaders));
        layout->addWidget(keyValue(QStringLiteral("Cookies"), record_.requestCookies));
        layout->addWidget(bodyWidget(QStringLiteral("Body"), record_.requestBody));
        return section;
    }

    QWidget* keyValue(const QString& title, const VarMap& map) {
        QWidget* w = new QWidget();
        QVBoxLayout* layout = new QVBoxLayout(w);
        layout->setContentsMargins(0, 0, 0, 0);
        QLabel* label = new QLabel(title + QStringLiteral(":"));
        label->setStyleSheet(QStringLiteral("color: %1;").arg(subtle().name()));
        layout->addWidget(label);

        QLabel* content = new QLabel();
        content->setTextInteractionFlags(Qt::TextSelectableByMouse);
        QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        font.setPointSize(9);
        content->setFont(font);
        QString text;
        if (map.empty()) {
            text = QStringLiteral("(vuoto)");
        } else {
            for (const auto& kv : map) {
                text += kv.first + QStringLiteral(": ") + kv.second + QStringLiteral("\n");
            }
        }
        content->setText(text);
        content->setWordWrap(false);
        layout->addWidget(content);
        return w;
    }

    QWidget* bodyWidget(const QString& title, const QString& body) {
        QWidget* w = new QWidget();
        QVBoxLayout* layout = new QVBoxLayout(w);
        layout->setContentsMargins(0, 0, 0, 0);

        QHBoxLayout* headerRow = new QHBoxLayout();
        QLabel* label = new QLabel(title + QStringLiteral(":"));
        label->setStyleSheet(QStringLiteral("color: %1;").arg(subtle().name()));
        headerRow->addWidget(label);
        headerRow->addStretch();

        if (body.isEmpty()) {
            QLabel* empty = new QLabel(QStringLiteral("(vuoto)"));
            layout->addWidget(empty);
            return w;
        }

        if (isJson(body)) {
            QPushButton* copy = new QPushButton(QStringLiteral("Copia JSON"));
            copy->setToolTip(QStringLiteral("Copia il contenuto di ") + title.toLower());
            connect(copy, &QPushButton::clicked, this, [body]() {
                QString pretty;
                try {
                    pretty = s2q(Json::parse(q2s(body)).dump(2));
                } catch (...) {
                    pretty = body;
                }
                QApplication::clipboard()->setText(pretty);
            });
            headerRow->addWidget(copy);
            layout->addLayout(headerRow);

            QTreeWidget* tree = new QTreeWidget();
            buildJsonTree(tree, body);
            tree->setFixedHeight(200);
            layout->addWidget(tree);
        } else {
            QPushButton* copy = new QPushButton(QStringLiteral("Copia"));
            copy->setToolTip(QStringLiteral("Copia il contenuto di ") + title.toLower());
            connect(copy, &QPushButton::clicked, this, [body]() {
                QApplication::clipboard()->setText(body);
            });
            headerRow->addWidget(copy);
            layout->addLayout(headerRow);

            QLabel* textLabel = new QLabel(body);
            textLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
            textLabel->setWordWrap(true);
            QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
            font.setPointSize(9);
            textLabel->setFont(font);
            textLabel->setMinimumHeight(100);
            layout->addWidget(textLabel);
        }
        return w;
    }

    CallRecord record_;
    QWidget* details_ = nullptr;
};

HistoryPanel::HistoryPanel(QWidget* parent) : QWidget(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    QWidget* toolbar = new QWidget(this);
    QHBoxLayout* toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(4, 4, 4, 4);
    QPushButton* clearButton = new QPushButton(QStringLiteral("Pulisci cronologia"), toolbar);
    clearButton->setToolTip(QStringLiteral("Cancella tutte le chiamate salvate in cronologia"));
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(clearButton);
    connect(clearButton, &QPushButton::clicked, this, &HistoryPanel::clearHistory);
    layout->addWidget(toolbar);

    QScrollArea* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    listPanel_ = new QWidget(scroll);
    QVBoxLayout* listLayout = new QVBoxLayout(listPanel_);
    listLayout->setContentsMargins(8, 8, 8, 8);
    listLayout->setSpacing(8);
    listLayout->addStretch();
    scroll->setWidget(listPanel_);
    layout->addWidget(scroll);
}

void HistoryPanel::refresh(const std::vector<CallRecord>& history) {
    QVBoxLayout* listLayout = qobject_cast<QVBoxLayout*>(listPanel_->layout());
    while (listLayout->count() > 1) {
        QLayoutItem* item = listLayout->takeAt(0);
        if (QWidget* w = item->widget()) w->deleteLater();
        delete item;
    }
    for (const auto& record : history) {
        listLayout->insertWidget(listLayout->count() - 1, new RecordCard(record, listPanel_));
    }
}

void HistoryPanel::clearHistory() {
    const QMessageBox::StandardButton res =
        QMessageBox::question(this, QStringLiteral("Pulisci cronologia"),
                              QStringLiteral("Cancellare tutta la cronologia delle chiamate?"),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (res == QMessageBox::Yes) {
        RcmRunService::instance().clearHistory();
    }
}

} // namespace rcm
