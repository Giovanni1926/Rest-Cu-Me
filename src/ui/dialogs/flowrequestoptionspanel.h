#pragma once

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QStackedWidget>
#include <QVBoxLayout>
#include "../widgets/collapsiblesection.h"
#include "../../util/str.h"

namespace rcm {

class FlowRequestOptionsPanel : public QWidget {
public:
    explicit FlowRequestOptionsPanel(QWidget* parent = nullptr);

    void loadFrom(const Json& call);
    void writeTo(Json& call);
    void refreshExpansion();
    QString validationError() const;

private:
    static int parsePositive(const QString& text);

    CollapsibleSection* authSection_ = nullptr;
    QComboBox* authCombo_ = nullptr;
    QStackedWidget* authCards_ = nullptr;
    QLineEdit* tokenField_ = nullptr;
    QLineEdit* usernameField_ = nullptr;
    QLineEdit* passwordField_ = nullptr;
    QLineEdit* keyField_ = nullptr;
    QLineEdit* valueField_ = nullptr;
    QComboBox* inCombo_ = nullptr;

    CollapsibleSection* timeoutSection_ = nullptr;
    QLineEdit* connectTimeoutField_ = nullptr;
    QLineEdit* readTimeoutField_ = nullptr;

    CollapsibleSection* tlsSection_ = nullptr;
    QComboBox* tlsVerificationCombo_ = nullptr;
    QLineEdit* caCertField_ = nullptr;
    QLineEdit* clientCertField_ = nullptr;
    QLineEdit* clientKeyField_ = nullptr;
    QLineEdit* keyPasswordField_ = nullptr;
};

} // namespace rcm
