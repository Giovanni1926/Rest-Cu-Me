#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QGroupBox>
#include "../widgets/keyvalueeditorpanel.h"
#include "../../util/str.h"

namespace rcm {

class FlowVariableNodeDialog : public QDialog {
public:
    FlowVariableNodeDialog(const QString& title, const VarMap& variables, bool editing,
                           QWidget* parent = nullptr)
        : QDialog(parent) {
        setWindowTitle(editing ? QStringLiteral("Modifica nodo variabili")
                               : QStringLiteral("Nuovo nodo variabili"));
        resize(540, 420);

        QVBoxLayout* layout = new QVBoxLayout(this);

        QGroupBox* titleGroup = new QGroupBox(QStringLiteral("Titolo (visibile sul canvas)"), this);
        QVBoxLayout* titleLayout = new QVBoxLayout(titleGroup);
        titleField_ = new QLineEdit(titleGroup);
        titleField_->setPlaceholderText(QStringLiteral("es. Parametri ambiente"));
        titleField_->setText(title);
        titleLayout->addWidget(titleField_);
        layout->addWidget(titleGroup);

        QGroupBox* varsGroup = new QGroupBox(QStringLiteral("Variabili"), this);
        QVBoxLayout* varsLayout = new QVBoxLayout(varsGroup);
        panel_ = new KeyValueEditorPanel(QStringLiteral("Variabile"), QStringLiteral("Valore"), false, varsGroup);
        varsLayout->addWidget(panel_);
        QLabel* hint1 = new QLabel(
            QStringLiteral("I valori possono usare ${variabile} (globali, collection o output di nodi a monte)."),
            varsGroup);
        QLabel* hint2 = new QLabel(
            QStringLiteral("Un link in uscita da questo nodo inietta tutte queste variabili nel nodo di destinazione."),
            varsGroup);
        QFont f = hint1->font();
        f.setPointSize(9);
        hint1->setFont(f);
        hint1->setStyleSheet(QStringLiteral("color: gray;"));
        hint1->setWordWrap(true);
        hint2->setFont(f);
        hint2->setStyleSheet(QStringLiteral("color: gray;"));
        hint2->setWordWrap(true);
        varsLayout->addWidget(hint1);
        varsLayout->addWidget(hint2);
        layout->addWidget(varsGroup);

        QHBoxLayout* buttons = new QHBoxLayout();
        QPushButton* ok = new QPushButton(QStringLiteral("OK"), this);
        QPushButton* cancel = new QPushButton(QStringLiteral("Annulla"), this);
        buttons->addStretch();
        buttons->addWidget(ok);
        buttons->addWidget(cancel);
        layout->addLayout(buttons);

        connect(ok, &QPushButton::clicked, this, &QDialog::accept);
        connect(cancel, &QPushButton::clicked, this, &QDialog::reject);

        panel_->setValues(variables);
    }

    QString getNodeTitle() const { return titleField_->text().trimmed(); }
    VarMap getVariables() const { return panel_->getValues(); }

private:
    QLineEdit* titleField_ = nullptr;
    KeyValueEditorPanel* panel_ = nullptr;
};

} // namespace rcm
