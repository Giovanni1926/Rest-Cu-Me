#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include "../widgets/keyvalueeditorpanel.h"
#include "../../util/str.h"

namespace rcm {

class FlowVariablesDialog : public QDialog {
public:
    FlowVariablesDialog(const Json& variables, QWidget* parent = nullptr) : QDialog(parent) {
        setWindowTitle(QStringLiteral("Variabili del flusso"));
        resize(520, 380);

        QVBoxLayout* layout = new QVBoxLayout(this);

        panel_ = new KeyValueEditorPanel(QStringLiteral("Variabile"), QStringLiteral("Valore"), false, this);
        layout->addWidget(panel_);

        QLabel* hint = new QLabel(
            QStringLiteral("Disponibili in tutti i nodi come ${nome}. Nei valori puoi usare ${altraVariabile}."),
            this);
        QFont f = hint->font();
        f.setPointSize(9);
        hint->setFont(f);
        hint->setStyleSheet(QStringLiteral("color: gray;"));
        hint->setWordWrap(true);
        layout->addWidget(hint);

        errorLabel_ = new QLabel(this);
        errorLabel_->setStyleSheet(QStringLiteral("color: red;"));
        errorLabel_->setVisible(false);
        layout->addWidget(errorLabel_);

        QHBoxLayout* buttons = new QHBoxLayout();
        QPushButton* ok = new QPushButton(QStringLiteral("OK"), this);
        QPushButton* cancel = new QPushButton(QStringLiteral("Annulla"), this);
        buttons->addStretch();
        buttons->addWidget(ok);
        buttons->addWidget(cancel);
        layout->addLayout(buttons);

        connect(ok, &QPushButton::clicked, this, [this]() {
            for (const auto& kv : panel_->getValues()) {
                if (kv.first.contains(QStringLiteral("${")) || kv.first.contains(QStringLiteral("}"))) {
                    errorLabel_->setText(QStringLiteral("Nome variabile non valido: ") + kv.first);
                    errorLabel_->setVisible(true);
                    return;
                }
            }
            accept();
        });
        connect(cancel, &QPushButton::clicked, this, &QDialog::reject);

        VarMap values;
        for (auto it = variables.begin(); it != variables.end(); ++it) {
            QString v;
            if (it.value().is_string()) v = s2q(it.value().get<std::string>());
            else if (!it.value().is_null()) v = s2q(it.value().dump());
            values.emplace_back(s2q(it.key()), v);
        }
        panel_->setValues(values);
    }

    VarMap getVariables() const { return panel_->getValues(); }

private:
    KeyValueEditorPanel* panel_ = nullptr;
    QLabel* errorLabel_ = nullptr;
};

} // namespace rcm
