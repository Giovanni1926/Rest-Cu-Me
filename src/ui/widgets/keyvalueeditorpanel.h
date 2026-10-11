#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <functional>
#include "../../util/str.h"

namespace rcm {

class KeyValueEditorPanel : public QWidget {
public:
    KeyValueEditorPanel(const QString& keyTitle, const QString& valueTitle, bool withPreview = false,
                        QWidget* parent = nullptr)
        : QWidget(parent), withPreview_(withPreview) {
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(4);

        table_ = new QTableWidget(0, withPreview_ ? 3 : 2, this);
        table_->setHorizontalHeaderLabels(withPreview_
                                              ? QStringList{keyTitle, valueTitle, QStringLiteral("Anteprima")}
                                              : QStringList{keyTitle, valueTitle});
        table_->setColumnWidth(0, 130);
        table_->setColumnWidth(1, withPreview_ ? 180 : 290);
        if (withPreview_) table_->setColumnWidth(2, 170);
        table_->setSelectionBehavior(QAbstractItemView::SelectRows);
        table_->setSelectionMode(QAbstractItemView::SingleSelection);
        table_->horizontalHeader()->setStretchLastSection(true);
        table_->setMinimumHeight(withPreview_ ? 130 : 110);
        table_->setMaximumHeight(withPreview_ ? 130 : 110);

        QHBoxLayout* buttonRow = new QHBoxLayout();
        buttonRow->setContentsMargins(0, 0, 0, 0);
        QPushButton* addButton = new QPushButton(QStringLiteral("Aggiungi"), this);
        QPushButton* removeButton = new QPushButton(QStringLiteral("Rimuovi"), this);
        buttonRow->addWidget(addButton);
        buttonRow->addWidget(removeButton);
        buttonRow->addStretch();

        layout->addWidget(table_);
        layout->addLayout(buttonRow);

        connect(addButton, &QPushButton::clicked, this, [this]() {
            table_->setCurrentCell(-1, -1);
            const int row = table_->rowCount();
            table_->insertRow(row);
            setCell(row, 0, QString());
            setCell(row, 1, QString());
            if (withPreview_) setCell(row, 2, QString());
            table_->setCurrentCell(row, 0);
            table_->editItem(table_->item(row, 0));
        });

        connect(removeButton, &QPushButton::clicked, this, [this]() {
            const int row = table_->currentRow();
            if (row >= 0 && row < table_->rowCount()) table_->removeRow(row);
        });

        connect(table_, &QTableWidget::itemChanged, this, [this]() { refreshPreviews(); });
    }

    VarMap getValues() const {
        stopEditing();
        VarMap values;
        for (int row = 0; row < table_->rowCount(); ++row) {
            const QString key = cellText(row, 0).trimmed();
            if (key.isEmpty()) continue;
            const QString value = cellText(row, 1).trimmed();
            put(values, key, value);
        }
        return values;
    }

    void setValues(const VarMap& values) {
        table_->setRowCount(0);
        int row = 0;
        for (const auto& kv : values) {
            table_->insertRow(row);
            setCell(row, 0, kv.first);
            setCell(row, 1, kv.second);
            if (withPreview_) setCell(row, 2, QString());
            ++row;
        }
        refreshPreviews();
    }

    void setPreviewProvider(const std::function<QString(const QString&)>& provider) {
        previewProvider_ = provider;
        refreshPreviews();
    }

    void refreshPreviews() {
        if (!withPreview_ || refreshing_ || !previewProvider_) return;
        refreshing_ = true;
        for (int row = 0; row < table_->rowCount(); ++row) {
            const QString value = previewProvider_(cellText(row, 0));
            if (cellText(row, 2) != value) setCell(row, 2, value);
        }
        refreshing_ = false;
    }

    QTableWidget* table() const { return table_; }

private:
    void stopEditing() const {
        QWidget* w = table_->focusWidget();
        Q_UNUSED(w);
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
        if (withPreview_ && col == 2) {
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        }
        item->setText(text);
    }

    QTableWidget* table_ = nullptr;
    bool withPreview_ = false;
    bool refreshing_ = false;
    std::function<QString(const QString&)> previewProvider_;
};

} // namespace rcm
