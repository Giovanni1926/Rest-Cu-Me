#pragma once

#include <QDialog>
#include <QLabel>
#include "../forms/restrequestform.h"

namespace rcm {

class FlowNodeDialog : public QDialog {
public:
    FlowNodeDialog(const QStringList& availableVariables, const Json& call, bool editing,
                   QWidget* parent = nullptr);

    Json getCall() const;
    QString getNodeTitle() const;

private:
    RestRequestForm* form_ = nullptr;
    QLabel* errorLabel_ = nullptr;
};

} // namespace rcm
