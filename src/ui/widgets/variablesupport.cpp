#include "variablesupport.h"

#include <QApplication>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QTableWidget>
#include <QListWidget>
#include <QFrame>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QTimer>
#include <QTextCursor>

namespace rcm {

VariableSupport::VariableSupport(const std::function<QStringList()>& variables, QObject* parent)
    : QObject(parent), variables_(variables) {}

QPushButton* VariableSupport::insertButton(QWidget* parent) {
    QPushButton* button = new QPushButton(QStringLiteral("Inserisci variabile"), parent);
    button->setToolTip(QStringLiteral("Inserisce ${variabile} nel campo o nell'editor attivo"));
    connect(button, &QPushButton::clicked, this, [this, button]() { showChooser(button, nullptr); });
    return button;
}

void VariableSupport::showChooser(QWidget* anchor, QPlainTextEdit* editor) {
    const QStringList vars = variables_();
    if (vars.isEmpty()) {
        QMessageBox::information(nullptr, QStringLiteral("Variabili"),
                                 QStringLiteral("Nessuna variabile disponibile nel flusso"));
        return;
    }

    QFrame* popup = new QFrame(nullptr, Qt::Popup);
    QVBoxLayout* layout = new QVBoxLayout(popup);
    layout->setContentsMargins(2, 2, 2, 2);
    QListWidget* list = new QListWidget(popup);
    list->addItems(vars);
    layout->addWidget(list);
    popup->resize(220, 220);

    QObject::connect(list, &QListWidget::itemDoubleClicked, popup, [popup, editor, this, list](QListWidgetItem* item) {
        const QString token = QStringLiteral("${") + item->text() + QStringLiteral("}");
        popup->close();
        popup->deleteLater();
        if (editor) {
            QTextCursor cursor = editor->textCursor();
            if (cursor.position() >= 2) {
                const QString before =
                    editor->toPlainText().mid(cursor.position() - 2, 2);
                if (before == QStringLiteral("${")) {
                    cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, 2);
                    editor->setTextCursor(cursor);
                    editor->textCursor().insertText(token);
                    editor->setFocus();
                    return;
                }
            }
            editor->textCursor().insertText(token);
            editor->setFocus();
        } else {
            insertIntoFocused(token);
        }
    });
    QObject::connect(list, &QListWidget::itemActivated, list,
                     [](QListWidgetItem*) {});

    QPoint global = anchor->mapToGlobal(QPoint(0, anchor->height()));
    popup->move(global);
    popup->show();
    list->setFocus();
}

void VariableSupport::insertIntoFocused(const QString& token) {
    QWidget* focus = QApplication::focusWidget();
    if (!focus) return;

    if (QPlainTextEdit* edit = qobject_cast<QPlainTextEdit*>(focus)) {
        edit->textCursor().insertText(token);
        return;
    }
    if (QLineEdit* edit = qobject_cast<QLineEdit*>(focus)) {
        edit->insert(token);
        return;
    }
    if (QTableWidget* table = qobject_cast<QTableWidget*>(focus)) {
        const int row = table->currentRow();
        const int col = table->currentColumn();
        if (row >= 0 && col >= 0) {
            QTableWidgetItem* item = table->item(row, col);
            if (!item) {
                item = new QTableWidgetItem();
                table->setItem(row, col, item);
            }
            item->setText(item->text() + token);
        }
    }
}

void VariableSupport::installPopup(QPlainTextEdit* editor) {
    connect(editor, &QPlainTextEdit::textChanged, this, [this, editor]() {
        const QTextCursor cursor = editor->textCursor();
        const int pos = cursor.position();
        const QString text = editor->toPlainText();
        if (pos >= 2 && text.mid(pos - 2, 2) == QStringLiteral("${")) {
            QTimer::singleShot(0, this, [this, editor]() { showChooser(editor, editor); });
        }
    });
}

} // namespace rcm
