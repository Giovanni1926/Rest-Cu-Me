#pragma once

#include <QString>
#include <functional>
#include <map>
#include <set>
#include <memory>
#include <pugixml.hpp>
#include "wsdldefinition.h"

namespace rcm {

class WsdlParser {
public:
    class WsdlException : public std::exception {
    public:
        explicit WsdlException(const QString& message) : message_(message.toUtf8().toStdString()) {}
        const char* what() const noexcept override { return message_.c_str(); }
        const QString message() const { return QString::fromStdString(message_); }

    private:
        std::string message_;
    };

    static WsdlDefinition parse(const QString& xml,
                                const std::function<QString(const QString&)>& importResolver);

private:
    struct MessagePart {
        pugi::xml_node node;
        QString name;
        QString element;
        QString type;
    };

    struct Schema {
        QString targetNamespace;
        std::map<QString, pugi::xml_node> elements;
    };

    explicit WsdlParser(const std::function<QString(const QString&)>& importResolver)
        : importResolver_(importResolver) {}

    pugi::xml_node parseIntoDoc(const QString& xml);

    void collect(const pugi::xml_node& definitions, int depth);
    void collectMessage(const pugi::xml_node& message);
    void collectTypes(const pugi::xml_node& types, int depth);
    void collectSchema(const pugi::xml_node& schema);
    void resolveSchemaLocation(const QString& location, int depth);
    void collectWsdlImport(const pugi::xml_node& importElement, int depth);
    QString fetchImport(const QString& location);
    WsdlDefinition build(const QString& definitionsTargetNamespace);

    static QString soapVersion(const pugi::xml_node& binding);
    static QString soapBindingStyle(const pugi::xml_node& binding);
    static QString endpoint(const pugi::xml_node& port, const QString& soapVersion);
    static QString soapAction(const pugi::xml_node& binding, const QString& operationName);

    pugi::xml_node inputElement(const std::vector<MessagePart>& parts);
    pugi::xml_node resolveElement(const MessagePart& part);
    std::vector<WsdlParameter> elementParameters(const pugi::xml_node& element);
    std::vector<WsdlParameter> partParameters(const std::vector<MessagePart>& parts);
    QString schemaNamespaceOf(const pugi::xml_node& element) const;
    bool elementFormDefaultQualified(const pugi::xml_node& element) const;

    std::function<QString(const QString&)> importResolver_;
    std::map<QString, std::vector<MessagePart>> messages_;
    std::map<QString, pugi::xml_node> bindings_;
    std::map<QString, pugi::xml_node> portTypes_;
    std::map<QString, Schema> schemas_;
    std::vector<pugi::xml_node> serviceElements_;
    std::set<QString> visitedImports_;
    std::vector<QString> warnings_;

    std::vector<std::shared_ptr<pugi::xml_document>> documents_;
};

} // namespace rcm
