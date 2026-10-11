#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include "../widgets/keyvalueeditorpanel.h"
#include "../../util/str.h"

namespace rcm {

class FlowCollectionVariablesDialog : public QDialog {
public:
    FlowCollectionVariablesDialog(const QString& groupName, const VarMap& variables,
                                  QWidget* parent = nullptr)
        : QDialog(parent) {
        setWindowTitle(QStringLiteral("Variabili della collection \"") + groupName +
                       QStringLiteral("\""));
        resize(520, 380);

        QVBoxLayout* layout = new QVBoxLayout(this);

        panel_ = new KeyValueEditorPanel(QStringLiteral("Variabile"), QStringLiteral("Valore"), false, this);
        layout->addWidget(panel_);

        QLabel* hint = new QLabel(
            QStringLiteral("Valori disponibili solo ai nodi di questa collection. Precedenza: output dei link > collection > globali."),
            this);
        QFont f = hint->font();
        f.setPointSize(9);
        hint->setFont(f);
        hint->setStyleSheet(QStringLiteral("color: gray;"));
        hint->setWordWrap(true);
        layout->addWidget(hint);

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

    VarMap getVariables() const { return panel_->getValues(); }

private:
    KeyValueEditorPanel* panel_ = nullptr;
};

} // namespace rcm
