#include "soaprequestform.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QFileDialog>
#include <QDir>
#include <QFile>
#include <QUrl>
#include <QFileInfo>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include "../widgets/collapsiblesection.h"
#include "../../model/wsdlparser.h"

namespace rcm {

static QString fetchWsdlUrl(const QString& urlString) {
    QNetworkAccessManager nam;
    QNetworkRequest request{QUrl(urlString)};
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
    QNetworkReply* reply = nam.get(request);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    const int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (code < 200 || code >= 300) {
        const QString err = QStringLiteral("HTTP ") + QString::number(code);
        reply->deleteLater();
        throw std::runtime_error(err.toStdString());
    }
    const QByteArray body = reply->readAll();
    reply->deleteLater();
    return QString::fromUtf8(body);
}

SoapRequestForm::SoapRequestForm(const QStringList& availableVariables, QWidget* parent)
    : QWidget(parent), availableVars_(availableVariables) {
    QVBoxLayout* contentLayout = new QVBoxLayout(this);
    contentLayout->setContentsMargins(8, 4, 4, 4);
    contentLayout->setSpacing(6);

    auto provider = [this]() { return availableVars_; };
    variableSupport_ = new VariableSupport(provider, this);

    QWidget* toolsRow = new QWidget(this);
    QHBoxLayout* toolsLayout = new QHBoxLayout(toolsRow);
    toolsLayout->setContentsMargins(0, 0, 0, 0);
    toolsLayout->addWidget(variableSupport_->insertButton(toolsRow));
    QLabel* hint = new QLabel(
        QStringLiteral("I placeholder ${nome} valgono in tutta la configurazione del nodo. Suggerite: variabili globali, di collection e output dei link in ingresso; digita ${ nell'XML."),
        toolsRow);
    QFont hf = hint->font();
    hf.setPointSize(9);
    hint->setFont(hf);
    hint->setStyleSheet(QStringLiteral("color: gray;"));
    hint->setWordWrap(true);
    toolsLayout->addWidget(hint, 1);
    contentLayout->addWidget(toolsRow);

    QWidget* wsdlContent = new QWidget();
    QVBoxLayout* wsdlLayout = new QVBoxLayout(wsdlContent);
    wsdlLayout->setContentsMargins(0, 4, 0, 0);
    wsdlSourceField_ = new QLineEdit(wsdlContent);
    wsdlSourceField_->setPlaceholderText(QStringLiteral("percorso file .wsdl oppure https://host/Service?wsdl"));
    QWidget* sourceRow = new QWidget();
    QHBoxLayout* sourceLayout = new QHBoxLayout(sourceRow);
    sourceLayout->setContentsMargins(0, 0, 0, 0);
    sourceLayout->addWidget(wsdlSourceField_, 1);
    QPushButton* browseButton = new QPushButton(QStringLiteral("Sfoglia..."), sourceRow);
    wsdlLoadButton_ = new QPushButton(QStringLiteral("Carica"), sourceRow);
    sourceLayout->addWidget(browseButton);
    sourceLayout->addWidget(wsdlLoadButton_);
    wsdlLayout->addWidget(new QLabel(QStringLiteral("Sorgente WSDL")));
    wsdlLayout->addWidget(sourceRow);

    serviceCombo_ = new QComboBox(wsdlContent);
    operationCombo_ = new QComboBox(wsdlContent);
    QPushButton* applyButton = new QPushButton(QStringLiteral("Applica operazione"), wsdlContent);
    wsdlLayout->addWidget(new QLabel(QStringLiteral("Servizio / porta")));
    wsdlLayout->addWidget(serviceCombo_);
    wsdlLayout->addWidget(new QLabel(QStringLiteral("Operazione")));
    wsdlLayout->addWidget(operationCombo_);
    wsdlLayout->addWidget(applyButton);

    wsdlStatusLabel_ = new QLabel(QStringLiteral(" "), wsdlContent);
    QFont sf = wsdlStatusLabel_->font();
    sf.setPointSize(9);
    wsdlStatusLabel_->setFont(sf);
    wsdlLayout->addWidget(wsdlStatusLabel_);

    CollapsibleSection* wsdlSection =
        new CollapsibleSection(QStringLiteral("WSDL (opzionale)"), wsdlContent, this);
    wsdlSection->setExpanded(true);
    contentLayout->addWidget(wsdlSection);

    connect(browseButton, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getOpenFileName(
            this, QStringLiteral("Scegli un file WSDL"), QString(), QStringLiteral("WSDL (*.wsdl)"));
        if (!path.isEmpty()) {
            wsdlSourceField_->setText(path);
            loadWsdl();
        }
    });
    connect(wsdlLoadButton_, &QPushButton::clicked, this, [this]() { loadWsdl(); });
    connect(applyButton, &QPushButton::clicked, this, [this]() { applySelectedOperation(); });
    connect(serviceCombo_, &QComboBox::currentIndexChanged, this,
            [this](int) { populateOperations(); });

    QGroupBox* headerGroup = new QGroupBox(QStringLiteral("Header"), this);
    QFormLayout* headerForm = new QFormLayout(headerGroup);
    titleField_ = new QLineEdit(headerGroup);
    titleField_->setPlaceholderText(QStringLiteral("es. GetUser"));
    urlField_ = new QLineEdit(headerGroup);
    urlField_->setPlaceholderText(QStringLiteral("https://host/Service"));
    urlField_->setMinimumWidth(440);
    headerForm->addRow(QStringLiteral("Titolo"), titleField_);
    headerForm->addRow(QStringLiteral("Endpoint URL"), urlField_);
    contentLayout->addWidget(headerGroup);

    QGroupBox* callGroup = new QGroupBox(QStringLiteral("Chiamata SOAP"), this);
    QFormLayout* callForm = new QFormLayout(callGroup);
    versionCombo_ = new QComboBox(callGroup);
    versionCombo_->addItems(QStringList{QStringLiteral("1.1"), QStringLiteral("1.2")});
    operationField_ = new QLineEdit(callGroup);
    operationField_->setPlaceholderText(QStringLiteral("es. GetUser"));
    namespaceField_ = new QLineEdit(callGroup);
    namespaceField_->setPlaceholderText(QStringLiteral("es. http://tempuri.org/"));
    qualifiedCombo_ = new QComboBox(callGroup);
    qualifiedCombo_->addItems(QStringList{QStringLiteral("Nel namespace (qualificati)"),
                                          QStringLiteral("Senza namespace")});
    actionField_ = new QLineEdit(callGroup);
    actionField_->setPlaceholderText(QStringLiteral("vuoto = namespace + operazione"));

    callForm->addRow(QStringLiteral("Versione"), versionCombo_);
    callForm->addRow(QStringLiteral("Operazione"), operationField_);
    callForm->addRow(QStringLiteral("Namespace"), namespaceField_);
    callForm->addRow(QStringLiteral("Parametri"), qualifiedCombo_);
    QLabel* qualifiedHint = new QLabel(
        QStringLiteral("Qualificati = figli nel namespace dell'operazione (tipico .NET/ASMX, da WSDL elementFormDefault)."),
        callGroup);
    QFont qf = qualifiedHint->font();
    qf.setPointSize(9);
    qualifiedHint->setFont(qf);
    qualifiedHint->setStyleSheet(QStringLiteral("color: gray;"));
    qualifiedHint->setWordWrap(true);
    callForm->addRow(qualifiedHint);
    callForm->addRow(QStringLiteral("SOAP Action"), actionField_);
    QLabel* actionHint = new QLabel(
        QStringLiteral("Vuoto = namespace + operazione. Con WSDL viene compilato da soapAction."),
        callGroup);
    QFont af = actionHint->font();
    af.setPointSize(9);
    actionHint->setFont(af);
    actionHint->setStyleSheet(QStringLiteral("color: gray;"));
    actionHint->setWordWrap(true);
    callForm->addRow(actionHint);

    parametersPanel_ = new KeyValueEditorPanel(QStringLiteral("Parametro"), QStringLiteral("Valore"), false, callGroup);
    QLabel* paramsHint = new QLabel(
        QStringLiteral("Un elemento XML per riga dentro l'operazione. Usa ${variabile} nei valori."),
        callGroup);
    QFont pf = paramsHint->font();
    pf.setPointSize(9);
    paramsHint->setFont(pf);
    paramsHint->setStyleSheet(QStringLiteral("color: gray;"));
    QVBoxLayout* callLayout = new QVBoxLayout(callGroup);
    callLayout->addLayout(callForm);
    callLayout->addWidget(parametersPanel_);
    callLayout->addWidget(paramsHint);
    contentLayout->addWidget(callGroup);

    QWidget* soapHeadersContent = new QWidget();
    QVBoxLayout* soapHeadersLayout = new QVBoxLayout(soapHeadersContent);
    soapHeadersLayout->setContentsMargins(0, 0, 0, 0);
    soapHeadersPanel_ = new KeyValueEditorPanel(QStringLiteral("Header SOAP"), QStringLiteral("Valore"), false, soapHeadersContent);
    QLabel* shHint = new QLabel(
        QStringLiteral("Elementi del blocco Header dell'envelope. Usa ${variabile} nei valori."),
        soapHeadersContent);
    QFont shf = shHint->font();
    shf.setPointSize(9);
    shHint->setFont(shf);
    shHint->setStyleSheet(QStringLiteral("color: gray;"));
    soapHeadersLayout->addWidget(soapHeadersPanel_);
    soapHeadersLayout->addWidget(shHint);
    CollapsibleSection* soapHeadersSection =
        new CollapsibleSection(QStringLiteral("Header SOAP (opzionale)"), soapHeadersContent, this);
    contentLayout->addWidget(soapHeadersSection);

    QWidget* bodyXmlContent = new QWidget();
    QVBoxLayout* bodyXmlLayout = new QVBoxLayout(bodyXmlContent);
    bodyXmlLayout->setContentsMargins(0, 0, 0, 0);
    bodyXmlArea_ = new CodeEditor(bodyXmlContent);
    bodyXmlArea_->setMode(CodeEditor::Xml);
    bodyXmlArea_->setMinimumSize(520, 150);
    variableSupport_->installPopup(bodyXmlArea_);
    QLabel* bxHint = new QLabel(
        QStringLiteral("Se compilato sostituisce l'intero envelope generato. Usa ${variabile}."),
        bodyXmlContent);
    QFont bxf = bxHint->font();
    bxf.setPointSize(9);
    bxHint->setFont(bxf);
    bxHint->setStyleSheet(QStringLiteral("color: gray;"));
    bodyXmlLayout->addWidget(bodyXmlArea_);
    bodyXmlLayout->addWidget(bxHint);
    CollapsibleSection* bodyXmlSection =
        new CollapsibleSection(QStringLiteral("XML avanzato (opzionale)"), bodyXmlContent, this);
    contentLayout->addWidget(bodyXmlSection);

    QWidget* httpHeadersContent = new QWidget();
    QVBoxLayout* httpHeadersLayout = new QVBoxLayout(httpHeadersContent);
    httpHeadersLayout->setContentsMargins(0, 0, 0, 0);
    httpHeadersPanel_ = new KeyValueEditorPanel(QStringLiteral("Header"), QStringLiteral("Valore"), false, httpHeadersContent);
    QLabel* hhHint = new QLabel(
        QStringLiteral("Header HTTP aggiuntivi. Content-Type e SOAPAction sono automatici."),
        httpHeadersContent);
    QFont hhf = hhHint->font();
    hhf.setPointSize(9);
    hhHint->setFont(hhf);
    hhHint->setStyleSheet(QStringLiteral("color: gray;"));
    httpHeadersLayout->addWidget(httpHeadersPanel_);
    httpHeadersLayout->addWidget(hhHint);
    CollapsibleSection* httpHeadersSection =
        new CollapsibleSection(QStringLiteral("Header HTTP (opzionale)"), httpHeadersContent, this);
    contentLayout->addWidget(httpHeadersSection);

    optionsPanel_ = new FlowRequestOptionsPanel(this);
    contentLayout->addWidget(optionsPanel_);
}

void SoapRequestForm::setAvailableVariables(const QStringList& variables) {
    availableVars_ = variables;
}

void SoapRequestForm::loadFrom(const Json& call) {
    auto str = [&](const char* key) -> QString {
        auto it = call.find(key);
        return (it != call.end() && it->is_string()) ? s2q(it->get<std::string>()) : QString();
    };
    urlField_->setText(str("url"));

    auto obj = [&](const char* key) -> VarMap {
        auto it = call.find(key);
        return (it != call.end() && it->is_object()) ? fromJsonObject(*it) : VarMap();
    };
    httpHeadersPanel_->setValues(obj("headers"));

    auto cookiesIt = call.find("cookies");
    if (cookiesIt != call.end() && cookiesIt->is_object()) {
        preservedCookies_ = fromJsonObject(*cookiesIt);
    }

    optionsPanel_->loadFrom(call);

    auto soapIt = call.find("soap");
    if (soapIt != call.end() && soapIt->is_object()) {
        const Json& soap = *soapIt;
        auto sstr = [&](const char* key) -> QString {
            auto it = soap.find(key);
            return (it != soap.end() && it->is_string()) ? s2q(it->get<std::string>()) : QString();
        };
        versionCombo_->setCurrentText(sstr("version").isEmpty() ? QStringLiteral("1.1") : sstr("version"));
        operationField_->setText(sstr("operation"));
        namespaceField_->setText(sstr("namespace"));
        actionField_->setText(sstr("action"));
        auto qualifiedIt = soap.find("qualified");
        bool qualified = qualifiedIt != soap.end() && qualifiedIt->is_boolean() && qualifiedIt->get<bool>();
        qualifiedCombo_->setCurrentIndex(qualified ? 0 : 1);
        bodyXmlArea_->setPlainText(sstr("bodyXml"));

        auto paramsIt = soap.find("parameters");
        if (paramsIt != soap.end() && paramsIt->is_object()) {
            parametersPanel_->setValues(fromJsonObject(*paramsIt));
        }
        auto shIt = soap.find("soapHeaders");
        if (shIt != soap.end() && shIt->is_object()) {
            soapHeadersPanel_->setValues(fromJsonObject(*shIt));
        }

        auto wsdlIt = soap.find("wsdl");
        if (wsdlIt != soap.end() && wsdlIt->is_object()) {
            auto wsrc = wsdlIt->find("source");
            if (wsrc != wsdlIt->end() && wsrc->is_string()) {
                wsdlSourceField_->setText(s2q(wsrc->get<std::string>()));
            }
            auto wsvc = wsdlIt->find("service");
            if (wsvc != wsdlIt->end() && wsvc->is_string()) {
                wsdlServiceSelection_ = s2q(wsvc->get<std::string>());
            }
            auto wop = wsdlIt->find("operation");
            if (wop != wsdlIt->end() && wop->is_string()) {
                wsdlOperationSelection_ = s2q(wop->get<std::string>());
            }
        }
    }
}

void SoapRequestForm::loadWsdl() {
    const QString source = wsdlSourceField_->text().trimmed();
    if (source.isEmpty()) {
        wsdlStatusLabel_->setText(QStringLiteral("Indica un file .wsdl o un URL"));
        wsdlStatusLabel_->setStyleSheet(QStringLiteral("color: red;"));
        return;
    }
    wsdlLoadButton_->setEnabled(false);
    wsdlStatusLabel_->setText(QStringLiteral("Caricamento..."));
    wsdlStatusLabel_->setStyleSheet(QStringLiteral("color: gray;"));

    struct WsdlFetchResult {
        QString xml;
        std::function<QString(const QString&)> resolver;
        QString error;
    };

    auto future = QtConcurrent::run([source]() {
        WsdlFetchResult result;
        try {
            if (source.startsWith(QStringLiteral("http://")) ||
                source.startsWith(QStringLiteral("https://"))) {
                result.xml = fetchWsdlUrl(source);
                result.resolver = [base = source](const QString& loc) {
                    QUrl baseUrl(base);
                    return fetchWsdlUrl(baseUrl.resolved(loc).toString());
                };
            } else {
                QFile f(source);
                if (!f.open(QIODevice::ReadOnly)) {
                    result.error = QStringLiteral("Impossibile leggere il file WSDL");
                    return result;
                }
                result.xml = QString::fromUtf8(f.readAll());
                f.close();
                const QDir dir = QFileInfo(source).absoluteDir();
                result.resolver = [dir](const QString& loc) {
                    const QString path = dir.filePath(loc);
                    QFile f(path);
                    if (!f.open(QIODevice::ReadOnly)) throw std::runtime_error("unresolved");
                    const QString content = QString::fromUtf8(f.readAll());
                    f.close();
                    return content;
                };
            }
        } catch (const std::exception& e) {
            result.error = QString::fromUtf8(e.what());
        }
        return result;
    });

    auto* watcher = new QFutureWatcher<WsdlFetchResult>();
    QObject::connect(watcher, &QFutureWatcher<WsdlFetchResult>::finished, watcher,
                     [this, watcher]() {
                         const WsdlFetchResult result = watcher->result();
                         watcher->deleteLater();
                         if (!result.error.isEmpty()) {
                             wsdlLoadButton_->setEnabled(true);
                             wsdlStatusLabel_->setText(result.error);
                             wsdlStatusLabel_->setStyleSheet(QStringLiteral("color: red;"));
                             return;
                         }
                         try {
                             WsdlDefinition def = WsdlParser::parse(result.xml, result.resolver);
                             applyDefinition(def);
                         } catch (const WsdlParser::WsdlException& e) {
                             wsdlLoadButton_->setEnabled(true);
                             wsdlStatusLabel_->setText(e.message());
                             wsdlStatusLabel_->setStyleSheet(QStringLiteral("color: red;"));
                         } catch (const std::exception& e) {
                             wsdlLoadButton_->setEnabled(true);
                             wsdlStatusLabel_->setText(QString::fromUtf8(e.what()));
                             wsdlStatusLabel_->setStyleSheet(QStringLiteral("color: red;"));
                         }
                     });
    watcher->setFuture(future);
}

void SoapRequestForm::applyDefinition(const WsdlDefinition& def) {
    wsdlDefinition_ = def;
    serviceCombo_->blockSignals(true);
    serviceCombo_->clear();
    if (def.services.empty()) {
        serviceCombo_->blockSignals(false);
        operationCombo_->clear();
        wsdlStatusLabel_->setText(QStringLiteral("Nessun servizio SOAP trovato nel WSDL"));
        wsdlStatusLabel_->setStyleSheet(QStringLiteral("color: red;"));
        wsdlLoadButton_->setEnabled(true);
        return;
    }
    for (const auto& service : def.services) {
        serviceCombo_->addItem(service.displayName());
    }
    int index = 0;
    if (!wsdlServiceSelection_.isEmpty()) {
        for (int i = 0; i < serviceCombo_->count(); ++i) {
            if (serviceCombo_->itemText(i) == wsdlServiceSelection_) {
                index = i;
                break;
            }
        }
    }
    serviceCombo_->setCurrentIndex(index);
    serviceCombo_->blockSignals(false);
    populateOperations();

    int opCount = 0;
    for (const auto& service : def.services) opCount += static_cast<int>(service.operations.size());
    QString status = QString::number(def.services.size()) + QStringLiteral(" servizi \u00b7 ") +
                     QString::number(opCount) + QStringLiteral(" operazioni");
    if (!def.warnings.empty()) status += QStringLiteral(" \u00b7 ") + QString::number(def.warnings.size()) + QStringLiteral(" avvisi");
    wsdlStatusLabel_->setText(status);
    wsdlStatusLabel_->setStyleSheet(QStringLiteral("color: gray;"));
    wsdlLoadButton_->setEnabled(true);
}

void SoapRequestForm::populateOperations() {
    if (!wsdlDefinition_) return;
    operationCombo_->blockSignals(true);
    operationCombo_->clear();
    const int serviceIndex = serviceCombo_->currentIndex();
    if (serviceIndex >= 0 && serviceIndex < static_cast<int>(wsdlDefinition_->services.size())) {
        const auto& service = wsdlDefinition_->services[serviceIndex];
        for (const auto& op : service.operations) {
            operationCombo_->addItem(op.signature());
        }
        int index = 0;
        if (!wsdlOperationSelection_.isEmpty()) {
            for (int i = 0; i < static_cast<int>(service.operations.size()); ++i) {
                if (service.operations[i].name == wsdlOperationSelection_) {
                    index = i;
                    break;
                }
            }
        }
        operationCombo_->setCurrentIndex(index);
    }
    operationCombo_->blockSignals(false);
}

void SoapRequestForm::applySelectedOperation() {
    if (!wsdlDefinition_) {
        wsdlStatusLabel_->setText(QStringLiteral("Carica un WSDL e scegli un'operazione"));
        wsdlStatusLabel_->setStyleSheet(QStringLiteral("color: red;"));
        return;
    }
    const int serviceIndex = serviceCombo_->currentIndex();
    if (serviceIndex < 0 || serviceIndex >= static_cast<int>(wsdlDefinition_->services.size())) {
        return;
    }
    const auto& service = wsdlDefinition_->services[serviceIndex];
    const int opIndex = operationCombo_->currentIndex();
    if (opIndex < 0 || opIndex >= static_cast<int>(service.operations.size())) {
        wsdlStatusLabel_->setText(QStringLiteral("Carica un WSDL e scegli un'operazione"));
        wsdlStatusLabel_->setStyleSheet(QStringLiteral("color: red;"));
        return;
    }
    const auto& operation = service.operations[opIndex];

    if (!service.endpoint.isEmpty()) urlField_->setText(service.endpoint);
    versionCombo_->setCurrentText(service.soapVersion);
    wsdlServiceSelection_ = service.displayName();
    operationField_->setText(operation.name);
    namespaceField_->setText(operation.ns);
    qualifiedCombo_->setCurrentIndex(operation.qualified ? 0 : 1);
    actionField_->setText(operation.soapAction);

    VarMap existing = parametersPanel_->getValues();
    for (const auto& param : operation.parameters) {
        bool found = false;
        for (const auto& kv : existing) {
            if (kv.first == param.name) {
                found = true;
                break;
            }
        }
        if (!found) existing.emplace_back(param.name, QString());
    }
    parametersPanel_->setValues(existing);

    wsdlOperationSelection_ = operation.name;
    wsdlStatusLabel_->setText(QStringLiteral("Operazione applicata: ") + operation.signature());
    wsdlStatusLabel_->setStyleSheet(QStringLiteral("color: gray;"));
}

QString SoapRequestForm::effectiveAction() const {
    const QString action = actionField_->text().trimmed();
    if (!action.isEmpty()) return action;
    const QString ns = namespaceField_->text().trimmed();
    const QString op = operationField_->text().trimmed();
    if (!ns.isEmpty() && !op.isEmpty()) {
        return ns + (ns.endsWith(QStringLiteral("/")) ? QString() : QStringLiteral("/")) + op;
    }
    return QString();
}

QString SoapRequestForm::validationError() const {
    if (urlField_->text().trimmed().isEmpty()) return QStringLiteral("Endpoint URL obbligatorio");
    if (bodyXmlArea_->toPlainText().trimmed().isEmpty() &&
        operationField_->text().trimmed().isEmpty()) {
        return QStringLiteral("Operazione obbligatoria (oppure compila l'XML avanzato)");
    }
    return optionsPanel_->validationError();
}

Json SoapRequestForm::getCall() const {
    Json call = Json::object();
    call["method"] = "POST";
    call["url"] = q2s(urlField_->text().trimmed());
    call["query"] = Json::object();
    call["headers"] = toJsonObject(httpHeadersPanel_->getValues());
    if (!preservedCookies_.empty()) call["cookies"] = toJsonObject(preservedCookies_);
    optionsPanel_->writeTo(call);
    call["body"] = "";
    call["contentType"] = "text/xml; charset=utf-8";

    Json soap = Json::object();
    soap["version"] = q2s(versionCombo_->currentText());
    soap["operation"] = q2s(operationField_->text().trimmed());
    soap["namespace"] = q2s(namespaceField_->text().trimmed());
    soap["qualified"] = qualifiedCombo_->currentIndex() == 0;
    soap["action"] = q2s(effectiveAction());
    soap["parameters"] = toJsonObject(parametersPanel_->getValues());
    soap["soapHeaders"] = toJsonObject(soapHeadersPanel_->getValues());
    soap["bodyXml"] = q2s(bodyXmlArea_->toPlainText().trimmed());

    const QString source = wsdlSourceField_->text().trimmed();
    if (!source.isEmpty()) {
        Json wsdl = Json::object();
        wsdl["source"] = q2s(source);
        wsdl["service"] = q2s(serviceCombo_->currentText());
        if (wsdlDefinition_) {
            const int idx = serviceCombo_->currentIndex();
            if (idx >= 0 && idx < static_cast<int>(wsdlDefinition_->services.size())) {
                const auto& service = wsdlDefinition_->services[idx];
                const int oi = operationCombo_->currentIndex();
                if (oi >= 0 && oi < static_cast<int>(service.operations.size())) {
                    wsdl["operation"] = q2s(service.operations[oi].name);
                }
            }
        }
        soap["wsdl"] = wsdl;
    }

    call["soap"] = soap;
    return call;
}

QString SoapRequestForm::getNodeTitle() const {
    return titleField_->text().trimmed();
}

} // namespace rcm
