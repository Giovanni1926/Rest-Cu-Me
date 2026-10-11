#include "wsdlparser.h"

namespace rcm {

static const QString WSDL_NS = QStringLiteral("http://schemas.xmlsoap.org/wsdl/");
static const QString WSDL2_NS = QStringLiteral("http://www.w3.org/ns/wsdl");
static const QString SOAP11_NS = QStringLiteral("http://schemas.xmlsoap.org/wsdl/soap/");
static const QString SOAP12_NS = QStringLiteral("http://schemas.xmlsoap.org/wsdl/soap12/");
static const QString XSD_NS = QStringLiteral("http://www.w3.org/2001/XMLSchema");
static const QString WSA_NS = QStringLiteral("http://www.w3.org/2006/05/addressing/wsdl");
static const int MAX_IMPORT_DEPTH = 5;

static QString localName(const pugi::xml_node& n) {
    const QString name = QString::fromUtf8(n.name());
    const int colon = name.indexOf(QLatin1Char(':'));
    return colon < 0 ? name : name.mid(colon + 1);
}

static QString prefixOf(const pugi::xml_node& n) {
    const QString name = QString::fromUtf8(n.name());
    const int colon = name.indexOf(QLatin1Char(':'));
    return colon < 0 ? QString() : name.left(colon);
}

static QString namespaceUri(const pugi::xml_node& n) {
    const QString prefix = prefixOf(n);
    for (pugi::xml_node cur = n; cur; cur = cur.parent()) {
        for (pugi::xml_attribute a = cur.first_attribute(); a; a = a.next_attribute()) {
            const QString an = QString::fromUtf8(a.name());
            if (prefix.isEmpty() && an == QStringLiteral("xmlns")) {
                return QString::fromUtf8(a.value());
            }
            if (!prefix.isEmpty() && an == QStringLiteral("xmlns:") + prefix) {
                return QString::fromUtf8(a.value());
            }
        }
    }
    return QString();
}

static QString lookupNamespace(const pugi::xml_node& node, const QString& prefix) {
    for (pugi::xml_node cur = node; cur; cur = cur.parent()) {
        for (pugi::xml_attribute a = cur.first_attribute(); a; a = a.next_attribute()) {
            const QString an = QString::fromUtf8(a.name());
            if (prefix.isEmpty() && an == QStringLiteral("xmlns")) {
                return QString::fromUtf8(a.value());
            }
            if (!prefix.isEmpty() && an == QStringLiteral("xmlns:") + prefix) {
                return QString::fromUtf8(a.value());
            }
        }
    }
    return QString();
}

static std::vector<pugi::xml_node> childElements(const pugi::xml_node& parent) {
    std::vector<pugi::xml_node> result;
    for (pugi::xml_node c = parent.first_child(); c; c = c.next_sibling()) {
        if (c.type() == pugi::node_element) result.push_back(c);
    }
    return result;
}

static std::vector<pugi::xml_node> children(const pugi::xml_node& parent, const QString& ns,
                                            const QString& local) {
    std::vector<pugi::xml_node> result;
    for (const auto& child : childElements(parent)) {
        if (localName(child) == local && namespaceUri(child) == ns) result.push_back(child);
    }
    return result;
}

static pugi::xml_node firstChild(const pugi::xml_node& parent, const QString& ns,
                                 const QString& local) {
    for (const auto& child : childElements(parent)) {
        if (localName(child) == local && namespaceUri(child) == ns) return child;
    }
    return pugi::xml_node();
}

static QString localNameOfQName(const QString& qname) {
    if (qname.isNull()) return QString();
    const int colon = qname.indexOf(QLatin1Char(':'));
    return colon < 0 ? qname : qname.mid(colon + 1);
}

pugi::xml_node WsdlParser::parseIntoDoc(const QString& xml) {
    auto doc = std::make_shared<pugi::xml_document>();
    const QByteArray bytes = xml.toUtf8();
    const pugi::xml_parse_result res = doc->load_string(bytes.constData());
    documents_.push_back(doc);
    if (!res) return pugi::xml_node();
    return doc->document_element();
}

pugi::xml_node WsdlParser::inputElement(const std::vector<MessagePart>& parts) {
    for (const auto& part : parts) {
        if (part.element.isEmpty()) continue;
        pugi::xml_node element = resolveElement(part);
        if (element) return element;
    }
    return pugi::xml_node();
}

pugi::xml_node WsdlParser::resolveElement(const MessagePart& part) {
    const QString qname = part.element;
    const QString local = localNameOfQName(qname);
    QString ns;
    const int colon = qname.indexOf(QLatin1Char(':'));
    ns = colon > 0 ? lookupNamespace(part.node, qname.left(colon))
                   : lookupNamespace(part.node, QString());

    auto schemaIt = schemas_.find(ns);
    if (schemaIt != schemas_.end()) {
        auto el = schemaIt->second.elements.find(local);
        if (el != schemaIt->second.elements.end()) return el->second;
    }
    for (const auto& candidate : schemas_) {
        auto el = candidate.second.elements.find(local);
        if (el != candidate.second.elements.end()) return el->second;
    }
    warnings_.push_back(QStringLiteral("Elemento '") + local +
                        QStringLiteral("' non trovato negli schemi inline"));
    return pugi::xml_node();
}

std::vector<WsdlParameter> WsdlParser::elementParameters(const pugi::xml_node& element) {
    std::vector<WsdlParameter> parameters;
    pugi::xml_node complexType = firstChild(element, XSD_NS, QStringLiteral("complexType"));
    if (!complexType) return parameters;
    pugi::xml_node container = firstChild(complexType, XSD_NS, QStringLiteral("sequence"));
    if (!container) container = firstChild(complexType, XSD_NS, QStringLiteral("all"));
    if (!container) container = firstChild(complexType, XSD_NS, QStringLiteral("choice"));
    if (!container) {
        pugi::xml_node complexContent =
            firstChild(complexType, XSD_NS, QStringLiteral("complexContent"));
        pugi::xml_node extension = complexContent
                                       ? firstChild(complexContent, XSD_NS, QStringLiteral("extension"))
                                       : pugi::xml_node();
        if (extension) {
            container = firstChild(extension, XSD_NS, QStringLiteral("sequence"));
            if (!container) container = firstChild(extension, XSD_NS, QStringLiteral("all"));
            if (!container) container = firstChild(extension, XSD_NS, QStringLiteral("choice"));
        }
    }
    if (!container) return parameters;
    for (const auto& child : children(container, XSD_NS, QStringLiteral("element"))) {
        const QString name = QString::fromUtf8(child.attribute("name").as_string());
        if (name.trimmed().isEmpty()) continue;
        QString type;
        const QString typeAttr = QString::fromUtf8(child.attribute("type").as_string());
        if (!typeAttr.isEmpty()) {
            type = localNameOfQName(typeAttr);
        } else if (firstChild(child, XSD_NS, QStringLiteral("complexType"))) {
            type = QStringLiteral("complexType");
        } else if (firstChild(child, XSD_NS, QStringLiteral("simpleType"))) {
            type = QStringLiteral("simpleType");
        } else {
            type = QStringLiteral("anyType");
        }
        parameters.emplace_back(name, type);
    }
    return parameters;
}

std::vector<WsdlParameter> WsdlParser::partParameters(const std::vector<MessagePart>& parts) {
    std::vector<WsdlParameter> parameters;
    for (const auto& part : parts) {
        if (part.name.trimmed().isEmpty()) continue;
        const QString type = !part.type.isEmpty() ? localNameOfQName(part.type)
                                                  : localNameOfQName(part.element);
        parameters.emplace_back(part.name, type);
    }
    return parameters;
}

QString WsdlParser::schemaNamespaceOf(const pugi::xml_node& element) const {
    pugi::xml_node parent = element.parent();
    if (parent && namespaceUri(parent) == XSD_NS && localName(parent) == QStringLiteral("schema")) {
        return QString::fromUtf8(parent.attribute("targetNamespace").as_string());
    }
    return QString();
}

bool WsdlParser::elementFormDefaultQualified(const pugi::xml_node& element) const {
    pugi::xml_node parent = element.parent();
    if (parent && namespaceUri(parent) == XSD_NS && localName(parent) == QStringLiteral("schema")) {
        return QString::fromUtf8(parent.attribute("elementFormDefault").as_string()) ==
               QStringLiteral("qualified");
    }
    return false;
}

QString WsdlParser::soapVersion(const pugi::xml_node& binding) {
    if (firstChild(binding, SOAP11_NS, QStringLiteral("binding"))) return QStringLiteral("1.1");
    if (firstChild(binding, SOAP12_NS, QStringLiteral("binding"))) return QStringLiteral("1.2");
    return QString();
}

QString WsdlParser::soapBindingStyle(const pugi::xml_node& binding) {
    pugi::xml_node soapBinding = firstChild(binding, SOAP11_NS, QStringLiteral("binding"));
    if (!soapBinding) soapBinding = firstChild(binding, SOAP12_NS, QStringLiteral("binding"));
    if (!soapBinding) return QStringLiteral("document");
    const QString style = QString::fromUtf8(soapBinding.attribute("style").as_string());
    return style.trimmed().isEmpty() ? QStringLiteral("document") : style;
}

QString WsdlParser::endpoint(const pugi::xml_node& port, const QString& soapVersion) {
    const QString preferred = soapVersion == QStringLiteral("1.2") ? SOAP12_NS : SOAP11_NS;
    const QString fallback = soapVersion == QStringLiteral("1.2") ? SOAP11_NS : SOAP12_NS;
    pugi::xml_node address = firstChild(port, preferred, QStringLiteral("address"));
    if (!address) address = firstChild(port, fallback, QStringLiteral("address"));
    if (!address) return QString();
    return QString::fromUtf8(address.attribute("location").as_string());
}

QString WsdlParser::soapAction(const pugi::xml_node& binding, const QString& operationName) {
    for (const auto& operation : children(binding, WSDL_NS, QStringLiteral("operation"))) {
        if (QString::fromUtf8(operation.attribute("name").as_string()) != operationName) continue;
        pugi::xml_node soapOperation = firstChild(operation, SOAP11_NS, QStringLiteral("operation"));
        if (!soapOperation) soapOperation = firstChild(operation, SOAP12_NS, QStringLiteral("operation"));
        if (soapOperation) {
            const QString action = QString::fromUtf8(soapOperation.attribute("soapAction").as_string());
            if (!action.trimmed().isEmpty()) return action;
        }
        pugi::xml_node input = firstChild(operation, WSDL_NS, QStringLiteral("input"));
        if (!input) continue;
        pugi::xml_node actionNode = firstChild(input, WSA_NS, QStringLiteral("Action"));
        if (actionNode) {
            const QString a = QString::fromUtf8(actionNode.text().as_string());
            if (!a.trimmed().isEmpty()) return a.trimmed();
        }
    }
    return QString();
}

WsdlDefinition WsdlParser::build(const QString& definitionsTargetNamespace) {
    std::vector<WsdlService> services;
    for (const auto& service : serviceElements_) {
        const QString serviceName = QString::fromUtf8(service.attribute("name").as_string());
        for (const auto& port : children(service, WSDL_NS, QStringLiteral("port"))) {
            const QString bindingName =
                localNameOfQName(QString::fromUtf8(port.attribute("binding").as_string()));
            auto bindingIt = bindings_.find(bindingName);
            if (bindingIt == bindings_.end()) continue;
            const QString soapVersionValue = soapVersion(bindingIt->second);
            if (soapVersionValue.isEmpty()) continue;
            const QString portTypeName =
                localNameOfQName(QString::fromUtf8(bindingIt->second.attribute("type").as_string()));
            auto portTypeIt = portTypes_.find(portTypeName);
            if (portTypeIt == portTypes_.end()) continue;

            const bool rpc = soapBindingStyle(bindingIt->second).compare(QStringLiteral("rpc"), Qt::CaseInsensitive) == 0;

            std::vector<WsdlOperation> operations;
            for (const auto& operation : children(portTypeIt->second, WSDL_NS, QStringLiteral("operation"))) {
                const QString name = QString::fromUtf8(operation.attribute("name").as_string());
                if (name.trimmed().isEmpty()) continue;
                pugi::xml_node input = firstChild(operation, WSDL_NS, QStringLiteral("input"));
                std::vector<MessagePart> parts;
                if (input) {
                    const QString msgName =
                        localNameOfQName(QString::fromUtf8(input.attribute("message").as_string()));
                    auto msgIt = messages_.find(msgName);
                    if (msgIt != messages_.end()) parts = msgIt->second;
                }
                QString ns = definitionsTargetNamespace;
                std::vector<WsdlParameter> parameters;
                bool qualified = false;
                if (rpc) {
                    parameters = partParameters(parts);
                } else {
                    pugi::xml_node element = inputElement(parts);
                    if (!element) {
                        parameters = partParameters(parts);
                    } else {
                        parameters = elementParameters(element);
                        qualified = elementFormDefaultQualified(element);
                        const QString schemaNs = schemaNamespaceOf(element);
                        if (!schemaNs.isNull() && !schemaNs.trimmed().isEmpty()) ns = schemaNs;
                    }
                }

                WsdlOperation op;
                op.name = name;
                op.soapAction = soapAction(bindingIt->second, name);
                op.ns = ns;
                op.style = rpc ? QStringLiteral("rpc") : QStringLiteral("document");
                op.qualified = qualified;
                op.parameters = parameters;
                operations.push_back(op);
            }

            WsdlService svc;
            svc.name = serviceName;
            svc.port = QString::fromUtf8(port.attribute("name").as_string());
            svc.endpoint = endpoint(port, soapVersionValue);
            svc.soapVersion = soapVersionValue;
            svc.operations = operations;
            services.push_back(svc);
        }
    }
    WsdlDefinition def;
    def.services = services;
    def.warnings = warnings_;
    return def;
}

void WsdlParser::collectMessage(const pugi::xml_node& message) {
    std::vector<MessagePart> parts;
    for (const auto& part : children(message, WSDL_NS, QStringLiteral("part"))) {
        MessagePart p;
        p.node = part;
        p.name = QString::fromUtf8(part.attribute("name").as_string());
        p.element = QString::fromUtf8(part.attribute("element").as_string());
        p.type = QString::fromUtf8(part.attribute("type").as_string());
        parts.push_back(p);
    }
    messages_[QString::fromUtf8(message.attribute("name").as_string())] = parts;
}

void WsdlParser::collectSchema(const pugi::xml_node& schema) {
    Schema& target = schemas_[QString::fromUtf8(schema.attribute("targetNamespace").as_string())];
    target.targetNamespace = QString::fromUtf8(schema.attribute("targetNamespace").as_string());
    for (const auto& element : children(schema, XSD_NS, QStringLiteral("element"))) {
        const QString name = QString::fromUtf8(element.attribute("name").as_string());
        if (name.trimmed().isEmpty()) continue;
        target.elements.emplace(name, element);
    }
}

void WsdlParser::collectTypes(const pugi::xml_node& types, int depth) {
    for (const auto& schema : children(types, XSD_NS, QStringLiteral("schema"))) {
        collectSchema(schema);
        if (depth >= MAX_IMPORT_DEPTH) continue;
        for (const auto& imported : children(schema, XSD_NS, QStringLiteral("import"))) {
            resolveSchemaLocation(QString::fromUtf8(imported.attribute("schemaLocation").as_string()), depth);
        }
        for (const auto& included : children(schema, XSD_NS, QStringLiteral("include"))) {
            resolveSchemaLocation(QString::fromUtf8(included.attribute("schemaLocation").as_string()), depth);
        }
    }
}

void WsdlParser::resolveSchemaLocation(const QString& location, int depth) {
    const QString xml = fetchImport(location);
    if (xml.isNull()) return;
    pugi::xml_node root = parseIntoDoc(xml);
    if (!root || namespaceUri(root) != XSD_NS || localName(root) != QStringLiteral("schema")) return;
    collectSchema(root);
    if (depth >= MAX_IMPORT_DEPTH) return;
    for (const auto& imported : children(root, XSD_NS, QStringLiteral("import"))) {
        resolveSchemaLocation(QString::fromUtf8(imported.attribute("schemaLocation").as_string()), depth + 1);
    }
    for (const auto& included : children(root, XSD_NS, QStringLiteral("include"))) {
        resolveSchemaLocation(QString::fromUtf8(included.attribute("schemaLocation").as_string()), depth + 1);
    }
}

void WsdlParser::collectWsdlImport(const pugi::xml_node& importElement, int depth) {
    if (depth >= MAX_IMPORT_DEPTH) return;
    const QString xml = fetchImport(QString::fromUtf8(importElement.attribute("location").as_string()));
    if (xml.isNull()) return;
    pugi::xml_node root = parseIntoDoc(xml);
    if (root && namespaceUri(root) == WSDL_NS) collect(root, depth + 1);
}

QString WsdlParser::fetchImport(const QString& location) {
    if (location.trimmed().isEmpty() || !importResolver_) return QString();
    if (!visitedImports_.insert(location).second) return QString();
    try {
        return importResolver_(location);
    } catch (...) {
        warnings_.push_back(QStringLiteral("Import non risolto: ") + location);
        return QString();
    }
}

void WsdlParser::collect(const pugi::xml_node& definitions, int depth) {
    for (const auto& child : childElements(definitions)) {
        if (namespaceUri(child) != WSDL_NS) continue;
        const QString local = localName(child);
        if (local == QStringLiteral("message")) {
            collectMessage(child);
        } else if (local == QStringLiteral("portType")) {
            portTypes_[QString::fromUtf8(child.attribute("name").as_string())] = child;
        } else if (local == QStringLiteral("binding")) {
            bindings_[QString::fromUtf8(child.attribute("name").as_string())] = child;
        } else if (local == QStringLiteral("service")) {
            serviceElements_.push_back(child);
        } else if (local == QStringLiteral("types")) {
            collectTypes(child, depth);
        } else if (local == QStringLiteral("import")) {
            collectWsdlImport(child, depth);
        }
    }
}

WsdlDefinition WsdlParser::parse(const QString& xml,
                                 const std::function<QString(const QString&)>& importResolver) {
    if (xml.isNull() || xml.trimmed().isEmpty()) {
        throw WsdlException(QStringLiteral("WSDL vuoto"));
    }
    try {
        WsdlParser parser(importResolver);
        pugi::xml_node root = parser.parseIntoDoc(xml);
        if (!root) throw WsdlException(QStringLiteral("WSDL non valido"));
        if (namespaceUri(root) == WSDL2_NS) {
            throw WsdlException(QStringLiteral("WSDL 2.0 non supportato: usa un WSDL 1.1"));
        }
        if (namespaceUri(root) != WSDL_NS || localName(root) != QStringLiteral("definitions")) {
            throw WsdlException(QStringLiteral("Documento non riconosciuto come WSDL 1.1"));
        }
        parser.collect(root, 0);
        return parser.build(QString::fromUtf8(root.attribute("targetNamespace").as_string()));
    } catch (const WsdlException& e) {
        throw;
    } catch (const std::exception& e) {
        throw WsdlException(QStringLiteral("WSDL non leggibile: ") + QString::fromUtf8(e.what()));
    }
}

} // namespace rcm
