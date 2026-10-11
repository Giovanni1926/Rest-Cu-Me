#include "flowedgedialog.h"
#include "outputsuggestions.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSplitter>
#include <QPlainTextEdit>
#include <QFontDatabase>
#include <QHeaderView>
#include <QListWidget>
#include <QFrame>
#include <QFont>
#include "../../flow/jsonpathextractor.h"

namespace rcm {

static QString nameFromExpression(const QString& expression) {
    const QString trimmed = expression.trimmed();
    if (trimmed.isEmpty() || trimmed == QStringLiteral("$")) return QString();
    QString name = trimmed.section(QLatin1Char('.'), -1);
    const int bracket = name.indexOf(QLatin1Char('['));
    if (bracket >= 0) name = name.left(bracket);
    name.remove(QLatin1Char('['));
    name.remove(QLatin1Char(']'));
    if (name == QStringLiteral("*") || name.isEmpty()) return QString();
    return name;
}

FlowEdgeDialog::FlowEdgeDialog(const FlowNode& source, const QString& targetLabel,
                               const std::vector<FlowOutput>& existingOutputs,
                               const CallResult* lastResult, bool isNew, QWidget* parent)
    : QDialog(parent), variableSource_(source.isVariableNode()) {
    setWindowTitle(isNew ? QStringLiteral("Nuovo link dati") : QStringLiteral("Modifica link dati"));
    resize(760, 460);

    QVBoxLayout* layout = new QVBoxLayout(this);

    QLabel* header = new QLabel(QStringLiteral("Da: ") + source.title +
                                QStringLiteral("   \u2192   A: ") + targetLabel, this);
    QFont hf = header->font();
    hf.setBold(true);
    header->setFont(hf);
    layout->addWidget(header);

    if (variableSource_) {
        QLabel* section = new QLabel(QStringLiteral("Variabili passate da \"") + source.title +
                                     QStringLiteral("\""), this);
        section->setStyleSheet(QStringLiteral("color: gray;"));
        layout->addWidget(section);

        table_ = new QTableWidget(0, 2, this);
        table_->setHorizontalHeaderLabels(QStringList{QStringLiteral("Variabile"), QStringLiteral("Valore")});
        table_->setColumnWidth(0, 160);
        table_->setColumnWidth(1, 340);
        table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table_->setSelectionBehavior(QAbstractItemView::SelectRows);
        int row = 0;
        for (const auto& kv : source.vars) {
            table_->insertRow(row);
            table_->setItem(row, 0, new QTableWidgetItem(QStringLiteral("${") + kv.first + QStringLiteral("}")));
            table_->setItem(row, 1, new QTableWidgetItem(kv.second));
            ++row;
        }
        table_->setMinimumHeight(300);
        layout->addWidget(table_);
    } else {
        QSplitter* splitter = new QSplitter(Qt::Horizontal, this);

        QWidget* left = new QWidget(splitter);
        QVBoxLayout* leftLayout = new QVBoxLayout(left);
        QLabel* leftTitle = new QLabel(QStringLiteral("Risposta di \"") + source.title +
                                           QStringLiteral("\" (ultima request)"), left);
        QFont lf = leftTitle->font();
        lf.setPointSize(9);
        lf.setBold(true);
        leftTitle->setFont(lf);
        leftLayout->addWidget(leftTitle);
        QPlainTextEdit* responseArea = new QPlainTextEdit(left);
        QFont rf = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        rf.setPointSize(10);
        responseArea->setFont(rf);
        responseArea->setReadOnly(true);
        responseArea->setLineWrapMode(QPlainTextEdit::NoWrap);
        responseArea->setPlainText(lastResult ? lastResult->detailed()
                                              : QStringLiteral("(esegui il flusso per vedere la risposta del nodo sorgente)"));
        leftLayout->addWidget(responseArea);
        splitter->addWidget(left);

        QWidget* right = new QWidget(splitter);
        QVBoxLayout* rightLayout = new QVBoxLayout(right);

        table_ = new QTableWidget(0, 3, right);
        table_->setHorizontalHeaderLabels(QStringList{QStringLiteral("Espressione"), QStringLiteral("Nome"),
                                                      QStringLiteral("Valore estratto")});
        table_->setColumnWidth(0, 180);
        table_->setColumnWidth(1, 120);
        table_->setColumnWidth(2, 200);
        table_->setSelectionBehavior(QAbstractItemView::SelectRows);
        table_->horizontalHeader()->setStretchLastSection(true);

        auto preview = [lastResult](const QString& expr) -> QString {
            if (!lastResult) return QStringLiteral("(esegui il flusso)");
            const QString value = JsonPathExtractor::extract(expr, *lastResult);
            if (value.isEmpty()) return QStringLiteral("(nessun valore)");
            if (value.size() > 60) return value.left(60) + QStringLiteral("\u2026");
            return value;
        };

        int row = 0;
        if (existingOutputs.empty()) {
            table_->insertRow(0);
            table_->setItem(0, 0, new QTableWidgetItem(QString()));
            table_->setItem(0, 1, new QTableWidgetItem(QString()));
            table_->setItem(0, 2, new QTableWidgetItem(preview(QString())));
        } else {
            for (const auto& out : existingOutputs) {
                table_->insertRow(row);
                table_->setItem(row, 0, new QTableWidgetItem(out.expr));
                table_->setItem(row, 1, new QTableWidgetItem(out.name));
                table_->setItem(row, 2, new QTableWidgetItem(preview(out.expr)));
                ++row;
            }
        }

        connect(table_, &QTableWidget::itemChanged, this, [this, preview]() {
            for (int r = 0; r < table_->rowCount(); ++r) {
                QTableWidgetItem* exprItem = table_->item(r, 0);
                const QString expr = exprItem ? exprItem->text() : QString();
                const QString value = preview(expr);
                QTableWidgetItem* valueItem = table_->item(r, 2);
                if (!valueItem) {
                    valueItem = new QTableWidgetItem();
                    valueItem->setFlags(valueItem->flags() & ~Qt::ItemIsEditable);
                    table_->setItem(r, 2, valueItem);
                }
                if (valueItem->text() != value) valueItem->setText(value);
            }
        });

        QHBoxLayout* tableButtons = new QHBoxLayout();
        QPushButton* addButton = new QPushButton(QStringLiteral("Aggiungi"), right);
        QPushButton* removeButton = new QPushButton(QStringLiteral("Rimuovi"), right);
        QPushButton* suggestionsButton = new QPushButton(QStringLiteral("Suggerimenti..."), right);
        tableButtons->addWidget(addButton);
        tableButtons->addWidget(removeButton);
        tableButtons->addWidget(suggestionsButton);
        tableButtons->addStretch();
        rightLayout->addWidget(table_);
        rightLayout->addLayout(tableButtons);

        QLabel* hint = new QLabel(
            source.isSoapNode()
                ? QStringLiteral("Per ogni riga: espressione XPath (es. //*[local-name()='Result']/text()), nome della variabile ${nome} e valore estratto.")
                : QStringLiteral("Per ogni riga: espressione JSONPath (es. $.id), nome della variabile ${nome} e valore estratto."),
            right);
        QFont hf2 = hint->font();
        hf2.setPointSize(9);
        hint->setFont(hf2);
        hint->setStyleSheet(QStringLiteral("color: gray;"));
        hint->setWordWrap(true);
        rightLayout->addWidget(hint);

        connect(addButton, &QPushButton::clicked, this, [this]() {
            const int r = table_->rowCount();
            table_->insertRow(r);
            table_->setItem(r, 0, new QTableWidgetItem(QString()));
            table_->setItem(r, 1, new QTableWidgetItem(QString()));
            table_->setItem(r, 2, new QTableWidgetItem(QString()));
            table_->setCurrentCell(r, 0);
        });
        connect(removeButton, &QPushButton::clicked, this, [this]() {
            const int r = table_->currentRow();
            if (r >= 0 && r < table_->rowCount()) table_->removeRow(r);
        });
        connect(suggestionsButton, &QPushButton::clicked, this, [this, lastResult, suggestionsButton]() {
            const QStringList suggestions = OutputSuggestions::fromResult(lastResult);
            QFrame* popup = new QFrame(nullptr, Qt::Popup);
            QVBoxLayout* pl = new QVBoxLayout(popup);
            pl->setContentsMargins(2, 2, 2, 2);
            QListWidget* list = new QListWidget(popup);
            list->addItems(suggestions);
            pl->addWidget(list);
            popup->resize(280, 220);
            QObject::connect(list, &QListWidget::itemClicked, popup, [this, popup, list](QListWidgetItem* item) {
                const QString suggestion = item->text();
                popup->close();
                popup->deleteLater();
                int row = table_->currentRow();
                if (row < 0) {
                    if (table_->rowCount() == 0) {
                        table_->insertRow(0);
                        table_->setItem(0, 0, new QTableWidgetItem(QString()));
                        table_->setItem(0, 1, new QTableWidgetItem(QString()));
                        table_->setItem(0, 2, new QTableWidgetItem(QString()));
                    }
                    row = table_->rowCount() - 1;
                }
                table_->setItem(row, 0, new QTableWidgetItem(suggestion));
                QTableWidgetItem* nameItem = table_->item(row, 1);
                if (!nameItem || nameItem->text().isEmpty()) {
                    table_->setItem(row, 1, new QTableWidgetItem(nameFromExpression(suggestion)));
                }
            });
            const QPoint global = suggestionsButton->mapToGlobal(QPoint(0, suggestionsButton->height()));
            popup->move(global);
            popup->show();
        });

        splitter->addWidget(right);
        splitter->setStretchFactor(0, 45);
        splitter->setStretchFactor(1, 55);
        splitter->setSizes({300, 360});
        layout->addWidget(splitter);
    }

    QHBoxLayout* buttons = new QHBoxLayout();
    QPushButton* ok = new QPushButton(QStringLiteral("OK"), this);
    QPushButton* cancel = new QPushButton(QStringLiteral("Annulla"), this);
    buttons->addStretch();
    buttons->addWidget(ok);
    buttons->addWidget(cancel);
    layout->addLayout(buttons);

    connect(ok, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
}

std::optional<std::vector<FlowOutput>> FlowEdgeDialog::getEdgeOutputs() const {
    if (variableSource_) return std::nullopt;

    std::vector<FlowOutput> outputs;
    for (int r = 0; r < table_->rowCount(); ++r) {
        QTableWidgetItem* exprItem = table_->item(r, 0);
        QTableWidgetItem* nameItem = table_->item(r, 1);
        QString expr = exprItem ? exprItem->text().trimmed() : QString();
        QString name = nameItem ? nameItem->text().trimmed() : QString();
        if (expr.isEmpty() && name.isEmpty()) continue;
        if (name.isEmpty()) name = nameFromExpression(expr);
        if (expr.isEmpty()) expr = QStringLiteral("$");
        outputs.emplace_back(name, expr);
    }
    return outputs;
}

} // namespace rcm
