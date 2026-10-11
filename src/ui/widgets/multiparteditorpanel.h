#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include "../../model/multipartpart.h"

namespace rcm {

class MultipartEditorPanel : public QWidget {
public:
    explicit MultipartEditorPanel(QWidget* parent = nullptr) : QWidget(parent) {
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(4);

        table_ = new QTableWidget(0, 5, this);
        table_->setHorizontalHeaderLabels(
            QStringList{QStringLiteral("Nome"), QStringLiteral("Tipo"),
                        QStringLiteral("Valore / Percorso"), QStringLiteral("Nome file"),
                        QStringLiteral("Content-Type")});
        table_->setColumnWidth(0, 90);
        table_->setColumnWidth(1, 60);
        table_->setColumnWidth(2, 190);
        table_->setColumnWidth(3, 100);
        table_->setColumnWidth(4, 130);
        table_->setSelectionBehavior(QAbstractItemView::SelectRows);
        table_->setSelectionMode(QAbstractItemView::SingleSelection);
        table_->horizontalHeader()->setStretchLastSection(true);
        table_->setMinimumHeight(140);
        table_->setMaximumHeight(140);

        QHBoxLayout* buttonRow = new QHBoxLayout();
        buttonRow->setContentsMargins(0, 0, 0, 0);
        QPushButton* addField = new QPushButton(QStringLiteral("Aggiungi campo"), this);
        QPushButton* addFile = new QPushButton(QStringLiteral("Aggiungi file..."), this);
        QPushButton* browse = new QPushButton(QStringLiteral("Sfoglia..."), this);
        QPushButton* remove = new QPushButton(QStringLiteral("Rimuovi"), this);
        buttonRow->addWidget(addField);
        buttonRow->addWidget(addFile);
        buttonRow->addWidget(browse);
        buttonRow->addWidget(remove);
        buttonRow->addStretch();

        layout->addWidget(table_);
        layout->addLayout(buttonRow);

        connect(addField, &QPushButton::clicked, this, [this]() {
            addRow(QStringLiteral("testo"), QString());
        });
        connect(addFile, &QPushButton::clicked, this, [this]() {
            const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Scegli un file"));
            if (!path.isEmpty()) addRow(QStringLiteral("file"), path);
        });
        connect(browse, &QPushButton::clicked, this, [this]() {
            const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Scegli un file"));
            if (path.isEmpty()) return;
            int row = table_->currentRow();
            if (row < 0 || row >= table_->rowCount()) {
                addRow(QStringLiteral("file"), path);
                return;
            }
            setCell(row, 1, QStringLiteral("file"));
            setCell(row, 2, path);
        });
        connect(remove, &QPushButton::clicked, this, [this]() {
            const int row = table_->currentRow();
            if (row >= 0 && row < table_->rowCount()) table_->removeRow(row);
        });
    }

    std::vector<MultipartPart> getValues() const {
        std::vector<MultipartPart> parts;
        for (int row = 0; row < table_->rowCount(); ++row) {
            const QString name = cellText(row, 0).trimmed();
            if (name.isEmpty()) continue;
            const QString type = cellText(row, 1).trimmed();
            if (type == MultipartPart::TYPE_FILE) {
                parts.emplace_back(name, MultipartPart::TYPE_FILE, QString(),
                                   cellText(row, 2).trimmed(), cellText(row, 3).trimmed(),
                                   cellText(row, 4).trimmed());
            } else {
                parts.emplace_back(name, MultipartPart::TYPE_TEXT, cellText(row, 2),
                                   QString(), QString(), cellText(row, 4).trimmed());
            }
        }
        return parts;
    }

    void setValues(const std::vector<MultipartPart>& parts) {
        table_->setRowCount(0);
        int row = 0;
        for (const auto& part : parts) {
            table_->insertRow(row);
            setCell(row, 0, part.name);
            setCell(row, 1, part.isFile() ? QStringLiteral("file") : QStringLiteral("testo"));
            setCell(row, 2, part.isFile() ? part.path : part.value);
            setCell(row, 3, part.fileName);
            setCell(row, 4, part.contentType);
            ++row;
        }
    }

private:
    void addRow(const QString& type, const QString& path) {
        const int row = table_->rowCount();
        table_->insertRow(row);
        setCell(row, 0, QString());
        setCell(row, 1, type);
        setCell(row, 2, path);
        setCell(row, 3, QString());
        setCell(row, 4, QString());
        table_->setCurrentCell(row, 0);
    }

    QString cellText(int row, int col) const {
        QTableWidgetItem* item = table_->item(row, col);
        return item ? item->text() : QString();
    }

    void setCell(int row, int col, const QString& text) {
        QTableWidgetItem* item = table_->item(row, col);
        if (!item) {
            item = new QTableWidgetItem();
            table_->setItem(row, col, item);
        }
        item->setText(text);
    }

    QTableWidget* table_ = nullptr;
};

} // namespace rcm
