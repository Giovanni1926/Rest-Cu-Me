#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QStringList>
#include <optional>
#include <functional>
#include "../widgets/keyvalueeditorpanel.h"
#include "../widgets/codeeditor.h"
#include "../widgets/variablesupport.h"
#include "../dialogs/flowrequestoptionspanel.h"
#include "../../model/wsdldefinition.h"
#include "../../util/str.h"

namespace rcm {

class SoapRequestForm : public QWidget {
public:
    explicit SoapRequestForm(const QStringList& availableVariables, QWidget* parent = nullptr);

    void loadFrom(const Json& call);
    Json getCall() const;
    QString validationError() const;
    QString getNodeTitle() const;
    void setAvailableVariables(const QStringList& variables);

private:
    void loadWsdl();
    void applyDefinition(const WsdlDefinition& def);
    void populateOperations();
    void applySelectedOperation();
    QString effectiveAction() const;

    QLineEdit* titleField_ = nullptr;
    QLineEdit* urlField_ = nullptr;
    QComboBox* versionCombo_ = nullptr;
    QLineEdit* operationField_ = nullptr;
    QLineEdit* namespaceField_ = nullptr;
    QComboBox* qualifiedCombo_ = nullptr;
    QLineEdit* actionField_ = nullptr;
    KeyValueEditorPanel* parametersPanel_ = nullptr;
    KeyValueEditorPanel* soapHeadersPanel_ = nullptr;
    CodeEditor* bodyXmlArea_ = nullptr;
    KeyValueEditorPanel* httpHeadersPanel_ = nullptr;
    FlowRequestOptionsPanel* optionsPanel_ = nullptr;

    QLineEdit* wsdlSourceField_ = nullptr;
    QComboBox* serviceCombo_ = nullptr;
    QComboBox* operationCombo_ = nullptr;
    QLabel* wsdlStatusLabel_ = nullptr;
    QPushButton* wsdlLoadButton_ = nullptr;

    VariableSupport* variableSupport_ = nullptr;
    VarMap preservedCookies_;
    QStringList availableVars_;

    std::optional<WsdlDefinition> wsdlDefinition_;
    QString wsdlServiceSelection_;
    QString wsdlOperationSelection_;
};

} // namespace rcm
