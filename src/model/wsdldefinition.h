#pragma once

#include <QString>
#include <vector>

namespace rcm {

struct WsdlParameter {
    QString name;
    QString type;

    WsdlParameter() = default;
    WsdlParameter(const QString& name, const QString& type)
        : name(name), type((type.isNull() || type.trimmed().isEmpty()) ? QStringLiteral("anyType") : type) {}
};

struct WsdlOperation {
    QString name;
    QString soapAction;
    QString ns;
    QString style;
    bool qualified = false;
    std::vector<WsdlParameter> parameters;

    WsdlOperation() = default;

    QString signature() const {
        QString sb = name + QStringLiteral("(");
        for (size_t i = 0; i < parameters.size(); ++i) {
            if (i) sb += QStringLiteral(", ");
            sb += parameters[i].name + QStringLiteral(": ") + parameters[i].type;
        }
        return sb + QStringLiteral(")");
    }
};

struct WsdlService {
    QString name;
    QString port;
    QString endpoint;
    QString soapVersion;
    std::vector<WsdlOperation> operations;

    WsdlService() = default;

    QString displayName() const {
        return (port.isNull() || port.trimmed().isEmpty()) ? name : name + QStringLiteral(" (") + port + QStringLiteral(")");
    }
};

struct WsdlDefinition {
    std::vector<WsdlService> services;
    std::vector<QString> warnings;
};

} // namespace rcm
