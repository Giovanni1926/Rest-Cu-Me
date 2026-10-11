#include "flowsoapnodedialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QScrollArea>

namespace rcm {

FlowSoapNodeDialog::FlowSoapNodeDialog(const QStringList& availableVariables, const Json& call,
                                       bool editing, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(editing ? QStringLiteral("Modifica nodo SOAP") : QStringLiteral("Nuovo nodo SOAP"));
    resize(640, 740);

    form_ = new SoapRequestForm(availableVariables, this);

    QWidget* content = new QWidget(this);
    QVBoxLayout* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(6);
    contentLayout->addWidget(form_);

    errorLabel_ = new QLabel(content);
    errorLabel_->setStyleSheet(QStringLiteral("color: red;"));
    errorLabel_->setVisible(false);
    contentLayout->addWidget(errorLabel_);

    QHBoxLayout* buttonRow = new QHBoxLayout();
    QPushButton* okButton = new QPushButton(QStringLiteral("OK"), content);
    QPushButton* cancelButton = new QPushButton(QStringLiteral("Annulla"), content);
    buttonRow->addStretch();
    buttonRow->addWidget(okButton);
    buttonRow->addWidget(cancelButton);
    contentLayout->addLayout(buttonRow);

    QScrollArea* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setWidget(content);

    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->addWidget(scroll);

    connect(okButton, &QPushButton::clicked, this, [this]() {
        const QString error = form_->validationError();
        if (!error.isEmpty()) {
            errorLabel_->setText(error);
            errorLabel_->setVisible(true);
            return;
        }
        accept();
    });
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    if (call.is_object()) {
        form_->loadFrom(call);
    }
}

Json FlowSoapNodeDialog::getCall() const {
    return form_->getCall();
}

QString FlowSoapNodeDialog::getNodeTitle() const {
    return form_->getNodeTitle();
}

} // namespace rcm
