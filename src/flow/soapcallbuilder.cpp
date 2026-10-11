#include "soapcallbuilder.h"

namespace rcm {

static QString stringOr(const Json& o, const QString& field, const QString& fallback) {
    if (!o.is_object()) return fallback;
    auto it = o.find(q2s(field));
    if (it == o.end() || it->is_null()) return fallback;
    if (it->is_string()) {
        QString v = s2q(it->get<std::string>());
        return v.trimmed().isEmpty() ? fallback : v;
    }
    return fallback;
}

static bool booleanOr(const Json& o, const QString& field, bool fallback) {
    if (!o.is_object()) return fallback;
    auto it = o.find(q2s(field));
    if (it == o.end() || !it->is_boolean()) return fallback;
    return it->get<bool>();
}

static Json objectOrNull(const Json& parent, const QString& field) {
    if (!parent.is_object()) return Json();
    auto it = parent.find(q2s(field));
    if (it == parent.end() || !it->is_object()) return Json();
    return *it;
}

QString SoapCallBuilder::tagName(const QString& name) {
    QString result;
    result.reserve(name.size());
    for (int i = 0; i < name.size(); ++i) {
        const QChar c = name.at(i);
        const bool ok = c.isLetterOrNumber() || c == QLatin1Char('_') ||
                        c == QLatin1Char('-') || c == QLatin1Char('.') ||
                        c == QLatin1Char(':');
        if (ok) {
            result += c;
        } else if (i != 0) {
            result += QLatin1Char('_');
        }
    }
    if (result.isEmpty()) return result;
    const QChar first = result.at(0);
    if (!first.isLetter() && first != QLatin1Char('_') && first != QLatin1Char(':')) {
        result = QLatin1Char('_') + result;
    }
    return result;
}

QString SoapCallBuilder::escape(const QString& value) {
    QString r = value;
    r.replace(QStringLiteral("&"), QStringLiteral("&amp;"));
    r.replace(QStringLiteral("<"), QStringLiteral("&lt;"));
    r.replace(QStringLiteral(">"), QStringLiteral("&gt;"));
    r.replace(QStringLiteral("\""), QStringLiteral("&quot;"));
    return r;
}

QString SoapCallBuilder::escapeAttribute(const QString& value) {
    QString r = escape(value);
    r.replace(QStringLiteral("'"), QStringLiteral("&apos;"));
    return r;
}

QString SoapCallBuilder::buildEnvelope(const QString& version, const Json& soap) {
    const bool soap12 = version == QStringLiteral("1.2");
    const QString envelopeNs = soap12 ? SOAP12_ENVELOPE_NS : SOAP11_ENVELOPE_NS;

    QString sb = QStringLiteral("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
    sb += QStringLiteral("<soap:Envelope xmlns:soap=\"") + envelopeNs + QStringLiteral("\">");

    Json headers = objectOrNull(soap, QStringLiteral("soapHeaders"));
    bool hasHeaders = headers.is_object() && !headers.empty();
    if (hasHeaders) {
        sb += QStringLiteral("<soap:Header>");
        for (auto it = headers.begin(); it != headers.end(); ++it) {
            QString tag = tagName(s2q(it.key()));
            if (tag.isEmpty()) continue;
            sb += QStringLiteral("<") + tag + QStringLiteral(">") +
                  escape(s2q(it.value().is_string() ? it.value().get<std::string>()
                                                    : it.value().dump())) +
                  QStringLiteral("</") + tag + QStringLiteral(">");
        }
        sb += QStringLiteral("</soap:Header>");
    }

    sb += QStringLiteral("<soap:Body>");

    const QString operation = stringOr(soap, QStringLiteral("operation"), QString());
    if (!operation.isEmpty()) {
        const QString ns = stringOr(soap, QStringLiteral("namespace"), QString());
        const bool qualified = booleanOr(soap, QStringLiteral("qualified"), false);
        QString open;
        if (ns.isEmpty()) {
            open = QStringLiteral("<") + tagName(operation) + QStringLiteral(">");
        } else if (qualified) {
            open = QStringLiteral("<") + tagName(operation) +
                   QStringLiteral(" xmlns=\"") + escapeAttribute(ns) + QStringLiteral("\">");
        } else {
            open = QStringLiteral("<tns:") + tagName(operation) +
                   QStringLiteral(" xmlns:tns=\"") + escapeAttribute(ns) + QStringLiteral("\">");
        }
        sb += open;

        Json params = objectOrNull(soap, QStringLiteral("parameters"));
        if (params.is_object()) {
            for (auto it = params.begin(); it != params.end(); ++it) {
                QString tag = tagName(s2q(it.key()));
                if (tag.isEmpty()) continue;
                sb += QStringLiteral("<") + tag + QStringLiteral(">") +
                      escape(s2q(it.value().is_string() ? it.value().get<std::string>()
                                                        : it.value().dump())) +
                      QStringLiteral("</") + tag + QStringLiteral(">");
            }
        }

        sb += QStringLiteral("</") + tagName(operation) + QStringLiteral(">");
    }

    sb += QStringLiteral("</soap:Body></soap:Envelope>");
    return sb;
}

Json SoapCallBuilder::prepare(const Json& call) {
    if (call.is_null() || !call.is_object()) return call;
    auto soapIt = call.find("soap");
    if (soapIt == call.end() || !soapIt->is_object()) return call;
    const Json& soap = *soapIt;

    const QString version = stringOr(soap, QStringLiteral("version"), QStringLiteral("1.1"));
    const bool soap12 = version == QStringLiteral("1.2");
    const QString action = stringOr(soap, QStringLiteral("action"), QString());

    Json prepared = call;
    prepared["method"] = "POST";
    if (soap12) {
        QString ct = SOAP12_CONTENT_TYPE;
        if (!action.isEmpty()) ct += QStringLiteral("; action=\"") + action + QStringLiteral("\"");
        prepared["contentType"] = q2s(ct);
    } else {
        prepared["contentType"] = q2s(SOAP11_CONTENT_TYPE);
    }

    Json headers = objectOrNull(prepared, QStringLiteral("headers"));
    if (!headers.is_object()) headers = Json::object();
    if (!soap12 && !action.isEmpty() && headers.find("SOAPAction") == headers.end()) {
        headers["SOAPAction"] = q2s(QStringLiteral("\"") + action + QStringLiteral("\""));
    }
    if (headers.find("Accept") == headers.end()) {
        headers["Accept"] = soap12 ? "application/soap+xml, text/xml" : "text/xml";
    }
    prepared["headers"] = headers;

    const QString bodyXml = stringOr(soap, QStringLiteral("bodyXml"), QString());
    prepared["body"] = bodyXml.trimmed().isEmpty() ? q2s(buildEnvelope(version, soap))
                                                   : q2s(bodyXml);
    return prepared;
}

} // namespace rcm
