#pragma once

#include <QDialog>
#include <QLabel>
#include "../forms/soaprequestform.h"

namespace rcm {

class FlowSoapNodeDialog : public QDialog {
public:
    FlowSoapNodeDialog(const QStringList& availableVariables, const Json& call, bool editing,
                       QWidget* parent = nullptr);

    Json getCall() const;
    QString getNodeTitle() const;

private:
    SoapRequestForm* form_ = nullptr;
    QLabel* errorLabel_ = nullptr;
};

} // namespace rcm
