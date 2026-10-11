#pragma once

#include <QString>
#include "../util/str.h"

namespace rcm {

class SoapCallBuilder {
public:
    static inline const QString SOAP11_ENVELOPE_NS =
        QStringLiteral("http://schemas.xmlsoap.org/soap/envelope/");
    static inline const QString SOAP12_ENVELOPE_NS =
        QStringLiteral("http://www.w3.org/2003/05/soap-envelope");
    static inline const QString SOAP11_CONTENT_TYPE = QStringLiteral("text/xml; charset=utf-8");
    static inline const QString SOAP12_CONTENT_TYPE =
        QStringLiteral("application/soap+xml; charset=utf-8");

    static Json prepare(const Json& call);
    static QString buildEnvelope(const QString& version, const Json& soap);

    static QString tagName(const QString& name);
    static QString escape(const QString& value);
    static QString escapeAttribute(const QString& value);
};

} // namespace rcm
