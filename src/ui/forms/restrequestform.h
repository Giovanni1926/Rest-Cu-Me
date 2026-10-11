#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QStackedWidget>
#include <QLabel>
#include <QStringList>
#include "../widgets/keyvalueeditorpanel.h"
#include "../widgets/multiparteditorpanel.h"
#include "../widgets/codeeditor.h"
#include "../widgets/variablesupport.h"
#include "../dialogs/flowrequestoptionspanel.h"
#include "../../model/bodyformat.h"
#include "../../util/str.h"

namespace rcm {

class RestRequestForm : public QWidget {
public:
    explicit RestRequestForm(const QStringList& availableVariables, QWidget* parent = nullptr);

    void loadFrom(const Json& call);
    Json getCall() const;
    QString validationError() const;
    QString getNodeTitle() const;
    void setAvailableVariables(const QStringList& variables);

private:
    void applyFormat(BodyFormat::Value format, bool userChange);
    QString hintFor(BodyFormat::Value format) const;

    QLineEdit* titleField_ = nullptr;
    QComboBox* methodCombo_ = nullptr;
    QLineEdit* urlField_ = nullptr;
    KeyValueEditorPanel* queryPanel_ = nullptr;
    KeyValueEditorPanel* headersPanel_ = nullptr;
    QComboBox* formatCombo_ = nullptr;
    QStackedWidget* formatCards_ = nullptr;
    CodeEditor* jsonEditor_ = nullptr;
    CodeEditor* xmlEditor_ = nullptr;
    CodeEditor* textEditor_ = nullptr;
    KeyValueEditorPanel* formPanel_ = nullptr;
    MultipartEditorPanel* multipartPanel_ = nullptr;
    QLabel* bodyHint_ = nullptr;
    QLineEdit* contentTypeField_ = nullptr;
    FlowRequestOptionsPanel* optionsPanel_ = nullptr;
    VariableSupport* variableSupport_ = nullptr;
    VarMap preservedCookies_;
    QStringList availableVars_;
};

} // namespace rcm
