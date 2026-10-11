#include "restrequestform.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include "../widgets/collapsiblesection.h"

namespace rcm {

RestRequestForm::RestRequestForm(const QStringList& availableVariables, QWidget* parent)
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
        QStringLiteral("I placeholder ${nome} valgono in tutta la configurazione del nodo. Suggerite: variabili globali, di collection e output dei link in ingresso; digita ${ negli editor."),
        toolsRow);
    QFont hf = hint->font();
    hf.setPointSize(9);
    hint->setFont(hf);
    hint->setStyleSheet(QStringLiteral("color: gray;"));
    hint->setWordWrap(true);
    toolsLayout->addWidget(hint, 1);
    contentLayout->addWidget(toolsRow);

    QGroupBox* headerGroup = new QGroupBox(QStringLiteral("Header"), this);
    QVBoxLayout* headerLayout = new QVBoxLayout(headerGroup);
    titleField_ = new QLineEdit(headerGroup);
    titleField_->setPlaceholderText(QStringLiteral("es. Ottieni utente"));
    methodCombo_ = new QComboBox(headerGroup);
    methodCombo_->addItems(QStringList{QStringLiteral("GET"), QStringLiteral("POST"),
                                       QStringLiteral("PUT"), QStringLiteral("PATCH"),
                                       QStringLiteral("DELETE")});
    urlField_ = new QLineEdit(headerGroup);
    urlField_->setMinimumWidth(440);

    QFormLayout* headerForm = new QFormLayout();
    headerForm->addRow(QStringLiteral("Titolo"), titleField_);
    QWidget* methodRow = new QWidget();
    QHBoxLayout* methodRowLayout = new QHBoxLayout(methodRow);
    methodRowLayout->setContentsMargins(0, 0, 0, 0);
    methodRowLayout->addWidget(methodCombo_);
    methodRowLayout->addWidget(urlField_, 1);
    headerForm->addRow(QStringLiteral("Metodo / URL"), methodRow);

    headersPanel_ = new KeyValueEditorPanel(QStringLiteral("Header"), QStringLiteral("Valore"), false, headerGroup);
    QLabel* headersHint = new QLabel(
        QStringLiteral("Header HTTP. Usa ${variabile} nei valori per iniettare l'output di un nodo a monte."),
        headerGroup);
    QFont hhf = headersHint->font();
    hhf.setPointSize(9);
    headersHint->setFont(hhf);
    headersHint->setStyleSheet(QStringLiteral("color: gray;"));
    headerLayout->addLayout(headerForm);
    headerLayout->addWidget(headersPanel_);
    headerLayout->addWidget(headersHint);
    contentLayout->addWidget(headerGroup);

    QGroupBox* bodyGroup = new QGroupBox(QStringLiteral("Body"), this);
    QVBoxLayout* bodyLayout = new QVBoxLayout(bodyGroup);
    formatCombo_ = new QComboBox(bodyGroup);
    formatCombo_->addItems(QStringList{QStringLiteral("Nessuno"), QStringLiteral("JSON"),
                                       QStringLiteral("XML"), QStringLiteral("Testo"),
                                       QStringLiteral("Form (x-www-form-urlencoded)"),
                                       QStringLiteral("Multipart/form-data")});

    formatCards_ = new QStackedWidget(bodyGroup);

    QWidget* noneCard = new QWidget();
    QVBoxLayout* noneLayout = new QVBoxLayout(noneCard);
    QLabel* noneLabel = new QLabel(QStringLiteral("Nessun body inviato per questo nodo."), noneCard);
    noneLabel->setAlignment(Qt::AlignCenter);
    noneLabel->setStyleSheet(QStringLiteral("color: gray;"));
    noneLabel->setMinimumHeight(50);
    noneLayout->addWidget(noneLabel);
    formatCards_->addWidget(noneCard);

    jsonEditor_ = new CodeEditor(formatCards_);
    jsonEditor_->setMode(CodeEditor::Json);
    jsonEditor_->setMinimumSize(520, 170);
    formatCards_->addWidget(jsonEditor_);

    xmlEditor_ = new CodeEditor(formatCards_);
    xmlEditor_->setMode(CodeEditor::Xml);
    xmlEditor_->setMinimumSize(520, 170);
    formatCards_->addWidget(xmlEditor_);

    textEditor_ = new CodeEditor(formatCards_);
    textEditor_->setMode(CodeEditor::Plain);
    textEditor_->setMinimumSize(520, 130);
    formatCards_->addWidget(textEditor_);

    formPanel_ = new KeyValueEditorPanel(QStringLiteral("Campo"), QStringLiteral("Valore"), false, formatCards_);
    formatCards_->addWidget(formPanel_);

    multipartPanel_ = new MultipartEditorPanel(formatCards_);
    formatCards_->addWidget(multipartPanel_);

    variableSupport_->installPopup(jsonEditor_);
    variableSupport_->installPopup(xmlEditor_);
    variableSupport_->installPopup(textEditor_);

    bodyHint_ = new QLabel(bodyGroup);
    QFont bhf = bodyHint_->font();
    bhf.setPointSize(9);
    bodyHint_->setFont(bhf);
    bodyHint_->setStyleSheet(QStringLiteral("color: gray;"));
    bodyHint_->setWordWrap(true);

    contentTypeField_ = new QLineEdit(bodyGroup);
    contentTypeField_->setText(QStringLiteral("application/json"));

    QFormLayout* bodyForm = new QFormLayout();
    bodyForm->addRow(QStringLiteral("Formato"), formatCombo_);
    bodyLayout->addLayout(bodyForm);
    bodyLayout->addWidget(formatCards_);
    bodyLayout->addWidget(bodyHint_);
    QWidget* ctRow = new QWidget(bodyGroup);
    QHBoxLayout* ctLayout = new QHBoxLayout(ctRow);
    ctLayout->setContentsMargins(0, 0, 0, 0);
    ctLayout->addWidget(new QLabel(QStringLiteral("Content-Type")));
    ctLayout->addWidget(contentTypeField_, 1);
    bodyLayout->addWidget(ctRow);
    contentLayout->addWidget(bodyGroup);

    queryPanel_ = new KeyValueEditorPanel(QStringLiteral("Parametro"), QStringLiteral("Valore"), false, this);
    QWidget* queryContent = new QWidget();
    QVBoxLayout* queryLayout = new QVBoxLayout(queryContent);
    queryLayout->setContentsMargins(0, 0, 0, 0);
    queryLayout->addWidget(queryPanel_);
    QLabel* queryHint = new QLabel(
        QStringLiteral("Parametri accodati all'URL (?chiave=valore). Usa ${variabile} nei valori."),
        queryContent);
    QFont qhf = queryHint->font();
    qhf.setPointSize(9);
    queryHint->setFont(qhf);
    queryHint->setStyleSheet(QStringLiteral("color: gray;"));
    queryLayout->addWidget(queryHint);
    CollapsibleSection* querySection =
        new CollapsibleSection(QStringLiteral("Query parameters"), queryContent, this);
    contentLayout->addWidget(querySection);

    optionsPanel_ = new FlowRequestOptionsPanel(this);
    contentLayout->addWidget(optionsPanel_);

    connect(formatCombo_, &QComboBox::currentIndexChanged, this, [this](int index) {
        applyFormat(static_cast<BodyFormat::Value>(index), true);
    });

    contentTypeField_->setText(QStringLiteral("application/json"));
    applyFormat(BodyFormat::JSON, false);
}

void RestRequestForm::setAvailableVariables(const QStringList& variables) {
    availableVars_ = variables;
}

QString RestRequestForm::hintFor(BodyFormat::Value format) const {
    switch (format) {
    case BodyFormat::NONE: return QStringLiteral("Nessun body inviato.");
    case BodyFormat::JSON:
        return QStringLiteral("Body JSON. Usa ${variabile} anche fuori dalle stringhe: la sostituzione avviene in esecuzione.");
    case BodyFormat::XML: return QStringLiteral("Body XML. Usa ${variabile} nei valori.");
    case BodyFormat::TEXT: return QStringLiteral("Testo libero. Usa ${variabile} dove serve.");
    case BodyFormat::FORM:
        return QStringLiteral("Coppie campo/valore codificate come x-www-form-urlencoded all'esecuzione.");
    case BodyFormat::MULTIPART:
        return QStringLiteral("Parti testo e file; i percorsi accettano ${variabile}. Il boundary è generato all'esecuzione.");
    }
    return QString();
}

void RestRequestForm::applyFormat(BodyFormat::Value format, bool userChange) {
    formatCombo_->setCurrentIndex(static_cast<int>(format));
    formatCards_->setCurrentIndex(static_cast<int>(format));
    if (userChange) {
        const BodyFormat bf(format);
        const QString ct = contentTypeField_->text();
        bool known = false;
        for (int v = BodyFormat::NONE; v <= BodyFormat::MULTIPART; ++v) {
            if (BodyFormat(static_cast<BodyFormat::Value>(v)).defaultContentType() == ct) {
                known = true;
                break;
            }
        }
        if (ct.isEmpty() || known) {
            contentTypeField_->setText(bf.defaultContentType());
        }
    }
    bodyHint_->setText(hintFor(format));
}

void RestRequestForm::loadFrom(const Json& call) {
    auto str = [&](const char* key) -> QString {
        auto it = call.find(key);
        return (it != call.end() && it->is_string()) ? s2q(it->get<std::string>()) : QString();
    };

    methodCombo_->setCurrentText(str("method").toUpper());
    urlField_->setText(str("url"));
    contentTypeField_->setText(str("contentType"));

    auto obj = [&](const char* key) -> VarMap {
        auto it = call.find(key);
        return (it != call.end() && it->is_object()) ? fromJsonObject(*it) : VarMap();
    };
    headersPanel_->setValues(obj("headers"));
    queryPanel_->setValues(obj("query"));

    auto cookiesIt = call.find("cookies");
    if (cookiesIt != call.end() && cookiesIt->is_object()) {
        preservedCookies_ = fromJsonObject(*cookiesIt);
    }

    optionsPanel_->loadFrom(call);

    auto partsIt = call.find("multipart");
    if (partsIt != call.end() && partsIt->is_array()) {
        std::vector<MultipartPart> parts;
        for (const auto& p : *partsIt) parts.push_back(MultipartPart::fromJson(p));
        multipartPanel_->setValues(parts);
    }

    const QString bodyFormat = [&]() {
        auto it = call.find("bodyFormat");
        return (it != call.end() && it->is_string()) ? s2q(it->get<std::string>()) : QString();
    }();

    const Json& body = call["body"];
    bool hasParts = partsIt != call.end() && partsIt->is_array() && !partsIt->empty();
    const QString bodyText = body.is_string() ? s2q(body.get<std::string>())
                                              : (body.is_object() ? s2q(body.dump()) : QString());
    BodyFormat::Value format = BodyFormat::infer(bodyFormat, str("contentType"),
                                                 (!bodyText.isEmpty()) || hasParts).value;

    switch (format) {
    case BodyFormat::JSON: jsonEditor_->setPlainText(bodyText); break;
    case BodyFormat::XML: xmlEditor_->setPlainText(bodyText); break;
    case BodyFormat::TEXT: textEditor_->setPlainText(bodyText); break;
    case BodyFormat::FORM:
        if (body.is_object()) formPanel_->setValues(fromJsonObject(body));
        break;
    case BodyFormat::MULTIPART: break;
    case BodyFormat::NONE: default: break;
    }

    applyFormat(format, false);
}

QString RestRequestForm::validationError() const {
    if (urlField_->text().trimmed().isEmpty()) return QStringLiteral("URL obbligatorio");
    return optionsPanel_->validationError();
}

Json RestRequestForm::getCall() const {
    Json call = Json::object();
    call["method"] = q2s(methodCombo_->currentText());
    call["url"] = q2s(urlField_->text().trimmed());
    call["query"] = toJsonObject(queryPanel_->getValues());
    call["headers"] = toJsonObject(headersPanel_->getValues());
    if (!preservedCookies_.empty()) call["cookies"] = toJsonObject(preservedCookies_);

    optionsPanel_->writeTo(call);

    const BodyFormat::Value format = static_cast<BodyFormat::Value>(formatCombo_->currentIndex());
    call["bodyFormat"] = q2s(BodyFormat(format).id());

    switch (format) {
    case BodyFormat::NONE:
        call["body"] = "";
        break;
    case BodyFormat::JSON: {
        const QString text = jsonEditor_->toPlainText();
        if (text.trimmed().isEmpty()) {
            call["body"] = "";
        } else {
            try {
                call["body"] = Json::parse(q2s(text));
            } catch (...) {
                call["body"] = q2s(text);
            }
        }
        break;
    }
    case BodyFormat::XML:
    case BodyFormat::TEXT: {
        const QString text = (format == BodyFormat::XML ? xmlEditor_->toPlainText()
                                                        : textEditor_->toPlainText())
                                 .trimmed();
        call["body"] = q2s(text);
        break;
    }
    case BodyFormat::FORM:
        call["body"] = toJsonObject(formPanel_->getValues());
        break;
    case BodyFormat::MULTIPART: {
        Json multipart = Json::array();
        for (const auto& part : multipartPanel_->getValues()) multipart.push_back(part.toJson());
        call["multipart"] = multipart;
        call["body"] = "";
        break;
    }
    }

    call["contentType"] = q2s(contentTypeField_->text().trimmed());
    return call;
}

QString RestRequestForm::getNodeTitle() const {
    return titleField_->text().trimmed();
}

} // namespace rcm
