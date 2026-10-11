#include "flowrequestoptionspanel.h"

#include <QLabel>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QGroupBox>

namespace rcm {

static QWidget* labeledField(const QString& label, QLineEdit* field) {
    QWidget* w = new QWidget();
    QHBoxLayout* l = new QHBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);
    l->addWidget(new QLabel(label));
    l->addWidget(field, 1);
    return w;
}

FlowRequestOptionsPanel::FlowRequestOptionsPanel(QWidget* parent) : QWidget(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    QWidget* authContent = new QWidget(this);
    QVBoxLayout* authLayout = new QVBoxLayout(authContent);
    authLayout->setContentsMargins(0, 4, 0, 0);
    authCombo_ = new QComboBox(authContent);
    authCombo_->addItems(QStringList{QStringLiteral("none"), QStringLiteral("bearer"),
                                     QStringLiteral("basic"), QStringLiteral("apikey")});
    authLayout->addWidget(authCombo_);

    authCards_ = new QStackedWidget(authContent);

    QWidget* nonePage = new QWidget();
    authCards_->addWidget(nonePage);

    QWidget* bearerPage = new QWidget();
    QVBoxLayout* bearerLayout = new QVBoxLayout(bearerPage);
    bearerLayout->setContentsMargins(0, 0, 0, 0);
    tokenField_ = new QLineEdit(bearerPage);
    bearerLayout->addWidget(labeledField(QStringLiteral("Token"), tokenField_));
    authCards_->addWidget(bearerPage);

    QWidget* basicPage = new QWidget();
    QVBoxLayout* basicLayout = new QVBoxLayout(basicPage);
    basicLayout->setContentsMargins(0, 0, 0, 0);
    usernameField_ = new QLineEdit(basicPage);
    passwordField_ = new QLineEdit(basicPage);
    basicLayout->addWidget(labeledField(QStringLiteral("Username"), usernameField_));
    basicLayout->addWidget(labeledField(QStringLiteral("Password"), passwordField_));
    authCards_->addWidget(basicPage);

    QWidget* apiPage = new QWidget();
    QVBoxLayout* apiLayout = new QVBoxLayout(apiPage);
    apiLayout->setContentsMargins(0, 0, 0, 0);
    keyField_ = new QLineEdit(apiPage);
    valueField_ = new QLineEdit(apiPage);
    inCombo_ = new QComboBox(apiPage);
    inCombo_->addItems(QStringList{QStringLiteral("header"), QStringLiteral("query")});
    apiLayout->addWidget(labeledField(QStringLiteral("Key"), keyField_));
    apiLayout->addWidget(labeledField(QStringLiteral("Value"), valueField_));
    QWidget* inRow = new QWidget();
    QHBoxLayout* inLayout = new QHBoxLayout(inRow);
    inLayout->setContentsMargins(0, 0, 0, 0);
    inLayout->addWidget(new QLabel(QStringLiteral("In")));
    inLayout->addWidget(inCombo_, 1);
    apiLayout->addWidget(inRow);
    authCards_->addWidget(apiPage);

    authLayout->addWidget(authCards_);
    authSection_ = new CollapsibleSection(QStringLiteral("Autenticazione"), authContent, this);
    layout->addWidget(authSection_);

    connect(authCombo_, &QComboBox::currentIndexChanged, this, [this](int index) {
        authCards_->setCurrentIndex(index);
    });

    QWidget* timeoutContent = new QWidget(this);
    QVBoxLayout* timeoutLayout = new QVBoxLayout(timeoutContent);
    timeoutLayout->setContentsMargins(0, 4, 0, 0);
    connectTimeoutField_ = new QLineEdit(timeoutContent);
    readTimeoutField_ = new QLineEdit(timeoutContent);
    timeoutLayout->addWidget(labeledField(QStringLiteral("Timeout connessione (s)"), connectTimeoutField_));
    timeoutLayout->addWidget(labeledField(QStringLiteral("Timeout risposta (s)"), readTimeoutField_));
    QLabel* timeoutHint = new QLabel(QStringLiteral("Vuoto = default (connessione 30s, risposta 60s)."), timeoutContent);
    QFont hf = timeoutHint->font();
    hf.setPointSize(9);
    timeoutHint->setFont(hf);
    timeoutHint->setStyleSheet(QStringLiteral("color: gray;"));
    timeoutLayout->addWidget(timeoutHint);
    timeoutSection_ = new CollapsibleSection(QStringLiteral("Timeout (opzionale)"), timeoutContent, this);
    layout->addWidget(timeoutSection_);

    QWidget* tlsContent = new QWidget(this);
    QVBoxLayout* tlsLayout = new QVBoxLayout(tlsContent);
    tlsLayout->setContentsMargins(0, 4, 0, 0);
    tlsVerificationCombo_ = new QComboBox(tlsContent);
    tlsVerificationCombo_->addItems(QStringList{QStringLiteral("Predefinita"),
                                                QStringLiteral("Accetta tutto (insecure)"),
                                                QStringLiteral("CA personalizzata")});
    tlsLayout->addWidget(tlsVerificationCombo_);

    caCertField_ = new QLineEdit(tlsContent);
    caCertField_->setPlaceholderText(QStringLiteral("percorso ca.pem"));
    clientCertField_ = new QLineEdit(tlsContent);
    clientCertField_->setPlaceholderText(QStringLiteral("percorso client.pem"));
    clientKeyField_ = new QLineEdit(tlsContent);
    clientKeyField_->setPlaceholderText(QStringLiteral("percorso client.key"));
    keyPasswordField_ = new QLineEdit(tlsContent);
    keyPasswordField_->setEchoMode(QLineEdit::Password);

    auto makeBrowseRow = [](QLineEdit* field, QWidget* parent) {
        QWidget* row = new QWidget(parent);
        QHBoxLayout* l = new QHBoxLayout(row);
        l->setContentsMargins(0, 0, 0, 0);
        l->addWidget(field, 1);
        QPushButton* browse = new QPushButton(QStringLiteral("Sfoglia..."), row);
        QObject::connect(browse, &QPushButton::clicked, field, [field]() {
            const QString path = QFileDialog::getOpenFileName(field, QStringLiteral("Scegli un file"));
            if (!path.isEmpty()) field->setText(path);
        });
        l->addWidget(browse);
        return row;
    };

    tlsLayout->addWidget(new QLabel(QStringLiteral("CA personalizzata (PEM)")));
    tlsLayout->addWidget(makeBrowseRow(caCertField_, tlsContent));
    tlsLayout->addWidget(new QLabel(QStringLiteral("Certificato client (PEM)")));
    tlsLayout->addWidget(makeBrowseRow(clientCertField_, tlsContent));
    tlsLayout->addWidget(new QLabel(QStringLiteral("Chiave privata client PKCS#8 (PEM)")));
    tlsLayout->addWidget(makeBrowseRow(clientKeyField_, tlsContent));
    tlsLayout->addWidget(labeledField(QStringLiteral("Password chiave"), keyPasswordField_));
    QLabel* tlsHint = new QLabel(QStringLiteral("Campi client opzionali (mTLS). Percorsi e password possono usare ${variabile}."), tlsContent);
    QFont tf = tlsHint->font();
    tf.setPointSize(9);
    tlsHint->setFont(tf);
    tlsHint->setStyleSheet(QStringLiteral("color: gray;"));
    tlsLayout->addWidget(tlsHint);
    tlsSection_ = new CollapsibleSection(QStringLiteral("SSL / Certificati (opzionale)"), tlsContent, this);
    layout->addWidget(tlsSection_);
}

int FlowRequestOptionsPanel::parsePositive(const QString& text) {
    if (text.trimmed().isEmpty()) return 0;
    bool ok = false;
    const int v = text.trimmed().toInt(&ok);
    return (ok && v > 0) ? v : -1;
}

void FlowRequestOptionsPanel::loadFrom(const Json& call) {
    if (!call.is_object()) return;

    auto authIt = call.find("auth");
    if (authIt != call.end() && authIt->is_object()) {
        const Json& auth = *authIt;
        auto typeIt = auth.find("type");
        QString type = typeIt != auth.end() && typeIt->is_string()
                           ? s2q(typeIt->get<std::string>())
                           : QStringLiteral("none");
        int index = authCombo_->findText(type);
        if (index < 0) index = 0;
        authCombo_->setCurrentIndex(index);

        auto setText = [&](QLineEdit* field, const char* key) {
            auto it = auth.find(key);
            field->setText(it != auth.end() && it->is_string() ? s2q(it->get<std::string>())
                                                               : QString());
        };
        setText(tokenField_, "token");
        setText(usernameField_, "username");
        setText(passwordField_, "password");
        setText(keyField_, "key");
        setText(valueField_, "value");
        auto inIt = auth.find("in");
        if (inIt != auth.end() && inIt->is_string()) {
            int inIndex = inCombo_->findText(s2q(inIt->get<std::string>()));
            if (inIndex >= 0) inCombo_->setCurrentIndex(inIndex);
        }
    }

    auto timeoutIt = call.find("timeout");
    if (timeoutIt != call.end() && timeoutIt->is_object()) {
        auto c = timeoutIt->find("connect");
        if (c != timeoutIt->end() && c->is_number_integer() && c->get<int>() > 0) {
            connectTimeoutField_->setText(QString::number(c->get<int>()));
        }
        auto r = timeoutIt->find("read");
        if (r != timeoutIt->end() && r->is_number_integer() && r->get<int>() > 0) {
            readTimeoutField_->setText(QString::number(r->get<int>()));
        }
    }

    auto tlsIt = call.find("tls");
    if (tlsIt != call.end() && tlsIt->is_object()) {
        const Json& tls = *tlsIt;
        auto v = tls.find("verification");
        QString verification = v != tls.end() && v->is_string()
                                   ? s2q(v->get<std::string>())
                                   : QStringLiteral("default");
        int index = 0;
        if (verification == QStringLiteral("trustAll")) index = 1;
        else if (verification == QStringLiteral("custom")) index = 2;
        tlsVerificationCombo_->setCurrentIndex(index);

        auto setText = [&](QLineEdit* field, const char* key) {
            auto it = tls.find(key);
            field->setText(it != tls.end() && it->is_string() ? s2q(it->get<std::string>())
                                                              : QString());
        };
        setText(caCertField_, "caCert");
        setText(clientCertField_, "clientCert");
        setText(clientKeyField_, "clientKey");
        setText(keyPasswordField_, "keyPassword");
    }

    refreshExpansion();
}

void FlowRequestOptionsPanel::writeTo(Json& call) {
    Json auth = Json::object();
    const QString type = authCombo_->currentText();
    auth["type"] = q2s(type);
    if (type == QStringLiteral("bearer")) {
        auth["token"] = q2s(tokenField_->text());
    } else if (type == QStringLiteral("basic")) {
        auth["username"] = q2s(usernameField_->text());
        auth["password"] = q2s(passwordField_->text());
    } else if (type == QStringLiteral("apikey")) {
        auth["key"] = q2s(keyField_->text());
        auth["value"] = q2s(valueField_->text());
        auth["in"] = q2s(inCombo_->currentText());
    }
    call["auth"] = auth;

    const int connect = parsePositive(connectTimeoutField_->text());
    const int read = parsePositive(readTimeoutField_->text());
    if (connect > 0 || read > 0) {
        Json timeout = Json::object();
        if (connect > 0) timeout["connect"] = connect;
        if (read > 0) timeout["read"] = read;
        call["timeout"] = timeout;
    }

    const QString verification =
        tlsVerificationCombo_->currentIndex() == 1
            ? QStringLiteral("trustAll")
            : (tlsVerificationCombo_->currentIndex() == 2 ? QStringLiteral("custom")
                                                         : QStringLiteral("default"));
    const QString caCert = caCertField_->text();
    const QString clientCert = clientCertField_->text();
    const QString clientKey = clientKeyField_->text();
    const QString keyPassword = keyPasswordField_->text();
    if (verification != QStringLiteral("default") || !caCert.isEmpty() ||
        !clientCert.isEmpty() || !clientKey.isEmpty() || !keyPassword.isEmpty()) {
        Json tls = Json::object();
        tls["verification"] = q2s(verification);
        if (!caCert.isEmpty()) tls["caCert"] = q2s(caCert);
        if (!clientCert.isEmpty()) tls["clientCert"] = q2s(clientCert);
        if (!clientKey.isEmpty()) tls["clientKey"] = q2s(clientKey);
        if (!keyPassword.isEmpty()) tls["keyPassword"] = q2s(keyPassword);
        call["tls"] = tls;
    }
}

void FlowRequestOptionsPanel::refreshExpansion() {
    authSection_->setExpanded(authCombo_->currentIndex() > 0);
    timeoutSection_->setExpanded(!connectTimeoutField_->text().trimmed().isEmpty() ||
                                 !readTimeoutField_->text().trimmed().isEmpty());
    tlsSection_->setExpanded(tlsVerificationCombo_->currentIndex() > 0 ||
                             !caCertField_->text().trimmed().isEmpty() ||
                             !clientCertField_->text().trimmed().isEmpty() ||
                             !clientKeyField_->text().trimmed().isEmpty());
}

QString FlowRequestOptionsPanel::validationError() const {
    const QString type = authCombo_->currentText();
    if (type == QStringLiteral("apikey") &&
        (keyField_->text().trimmed().isEmpty() || valueField_->text().trimmed().isEmpty())) {
        return QStringLiteral("Auth apikey: key e value obbligatori");
    }
    if (type == QStringLiteral("bearer") && tokenField_->text().trimmed().isEmpty()) {
        return QStringLiteral("Auth bearer: token obbligatorio");
    }
    if (!connectTimeoutField_->text().trimmed().isEmpty() &&
        parsePositive(connectTimeoutField_->text()) < 0) {
        return QStringLiteral("Timeout connessione non valido: usa un numero di secondi > 0");
    }
    if (!readTimeoutField_->text().trimmed().isEmpty() &&
        parsePositive(readTimeoutField_->text()) < 0) {
        return QStringLiteral("Timeout risposta non valido: usa un numero di secondi > 0");
    }
    if (tlsVerificationCombo_->currentIndex() == 2 && caCertField_->text().trimmed().isEmpty()) {
        return QStringLiteral("Verifica CA personalizzata: indica il file PEM della CA");
    }
    if (clientCertField_->text().trimmed().isEmpty() != clientKeyField_->text().trimmed().isEmpty()) {
        return QStringLiteral("Certificato client: servono sia certificato sia chiave (PEM)");
    }
    return QString();
}

} // namespace rcm
