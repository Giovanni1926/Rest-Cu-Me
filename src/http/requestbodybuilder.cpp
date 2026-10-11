#include "requestbodybuilder.h"
#include "restclient.h"
#include <QUuid>
#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QStringList>

namespace rcm {

static const int MAX_SNAPSHOT_VALUE = 80;

static QString jsonPrimitiveString(const Json& b) {
    if (b.is_string()) return s2q(b.get<std::string>());
    return s2q(b.dump());
}

QString RequestBodyBuilder::quoted(const QString& value) {
    QString r = value;
    r.replace(QStringLiteral("\""), QStringLiteral("%22"));
    r.replace(QStringLiteral("\r"), QStringLiteral(" "));
    r.replace(QStringLiteral("\n"), QStringLiteral(" "));
    return r;
}

QString RequestBodyBuilder::abbreviate(const QString& value) {
    if (value.isEmpty()) return QString();
    if (value.size() <= MAX_SNAPSHOT_VALUE) return value;
    return value.left(77) + QStringLiteral("...");
}

RequestBodyBuilder::BuiltBody RequestBodyBuilder::buildText(const QString& text) {
    BuiltBody b;
    b.contentType = QString();
    b.body = text.toUtf8();
    b.snapshot = text;
    return b;
}

RequestBodyBuilder::BuiltBody RequestBodyBuilder::buildJson(
    const Json& body, const std::function<QString(const QString&)>& substitute) {
    if (body.is_null()) return buildText(substitute(QString()));
    if (body.is_string()) return buildText(substitute(s2q(body.get<std::string>())));
    Json substituted = substituteJson(body, substitute);
    return buildText(s2q(substituted.dump()));
}

RequestBodyBuilder::BuiltBody RequestBodyBuilder::asText(
    const Json& body, const std::function<QString(const QString&)>& substitute) {
    if (body.is_null()) return buildText(QString());
    if (body.is_object() || body.is_array()) {
        return buildText(s2q(body.dump()));
    }
    return buildText(substitute(jsonPrimitiveString(body)));
}

RequestBodyBuilder::BuiltBody RequestBodyBuilder::encodeForm(
    const Json& body, const std::function<QString(const QString&)>& substitute) {
    if (body.is_object()) {
        QStringList pairs;
        for (auto it = body.begin(); it != body.end(); ++it) {
            const QString key = substitute(s2q(it.key()));
            if (key.trimmed().isEmpty()) continue;
            QString text;
            if (it.value().is_null()) {
                text = QString();
            } else if (it.value().is_string()) {
                text = substitute(s2q(it.value().get<std::string>()));
            } else {
                text = substitute(s2q(it.value().dump()));
            }
            pairs << RestClient::urlEncode(key) + QStringLiteral("=") +
                         RestClient::urlEncode(text);
        }
        return buildText(pairs.join(QStringLiteral("&")));
    }
    return buildText(substitute(body.is_null() ? QString() : jsonPrimitiveString(body)));
}

RequestBodyBuilder::BuiltBody RequestBodyBuilder::buildMultipart(
    const RestCallDef& call, const std::function<QString(const QString&)>& substitute) {
    const QString boundary = QStringLiteral("----RestCuMe") +
                             QUuid::createUuid().toString(QUuid::WithoutBraces).remove(QLatin1Char('-'));

    QByteArray body;
    QString snapshot = QStringLiteral("multipart/form-data; boundary=") + boundary;

    for (const MultipartPart& part : call.multipart) {
        if (part.name.trimmed().isEmpty()) continue;
        const QString name = substitute(part.name);
        snapshot += QStringLiteral("\n- ") + name + (part.isFile() ? QStringLiteral(" (file)")
                                                                  : QStringLiteral(" (testo)"));
        if (part.isFile()) {
            const QString resolvedPath = substitute(part.path);
            QFile file(resolvedPath);
            if (!file.exists()) {
                throw std::runtime_error(
                    ("File multipart non trovato: " + resolvedPath).toStdString());
            }
            const QString fileName = part.fileName.isEmpty()
                                         ? QFileInfo(file).fileName()
                                         : substitute(part.fileName);
            QString partType = substitute(part.contentType);
            if (partType.isEmpty()) {
                QMimeDatabase db;
                partType = db.mimeTypeForFile(resolvedPath).name();
            }
            snapshot += QStringLiteral(" \u2192 ") + resolvedPath + QStringLiteral(" (") +
                        fileName + QStringLiteral(")");
            body += headBytes(boundary, name, fileName, partType);
            if (!file.open(QIODevice::ReadOnly)) {
                throw std::runtime_error(
                    ("File multipart non trovato: " + resolvedPath).toStdString());
            }
            body += file.readAll();
            file.close();
            body += "\r\n";
        } else {
            const QString value = substitute(part.value);
            snapshot += QStringLiteral(" = ") + abbreviate(value);
            const QString partType = substitute(part.contentType);
            body += headBytes(boundary, name, QString(), partType);
            body += value.toUtf8();
            body += "\r\n";
        }
    }

    body += "--" + boundary.toUtf8() + "--\r\n";

    QString base;
    const QString ct = call.contentType;
    if (ct.trimmed().isEmpty() || !ct.toLower().startsWith(QStringLiteral("multipart"))) {
        base = QStringLiteral("multipart/form-data");
    } else {
        base = ct.section(QLatin1Char(';'), 0, 0).trimmed();
    }

    BuiltBody b;
    b.contentType = base + QStringLiteral("; boundary=") + boundary;
    b.body = body;
    b.snapshot = snapshot;
    return b;
}

QByteArray RequestBodyBuilder::headBytes(const QString& boundary, const QString& name,
                                         const QString& fileName, const QString& contentType) {
    QString head = QStringLiteral("--") + boundary + QStringLiteral("\r\n");
    head += QStringLiteral("Content-Disposition: form-data; name=\"") + quoted(name) +
            QStringLiteral("\"");
    if (!fileName.isEmpty()) {
        head += QStringLiteral("; filename=\"") + quoted(fileName) + QStringLiteral("\"");
    }
    head += QStringLiteral("\r\n");
    if (!contentType.isEmpty()) {
        head += QStringLiteral("Content-Type: ") + contentType + QStringLiteral("\r\n");
    }
    head += QStringLiteral("\r\n");
    return head.toUtf8();
}

Json RequestBodyBuilder::substituteJson(
    const Json& element, const std::function<QString(const QString&)>& substitute) {
    if (element.is_null()) return element;
    if (element.is_string()) {
        return substitute(s2q(element.get<std::string>())).toUtf8().toStdString();
    }
    if (element.is_number() || element.is_boolean()) return element;
    if (element.is_array()) {
        Json result = Json::array();
        for (const auto& item : element) {
            result.push_back(substituteJson(item, substitute));
        }
        return result;
    }
    if (element.is_object()) {
        Json result = Json::object();
        for (auto it = element.begin(); it != element.end(); ++it) {
            result[q2s(substitute(s2q(it.key())))] = substituteJson(it.value(), substitute);
        }
        return result;
    }
    return element;
}

RequestBodyBuilder::BuiltBody RequestBodyBuilder::build(
    const RestCallDef& call, const std::function<QString(const QString&)>& substitute) {
    switch (call.format.value) {
    case BodyFormat::MULTIPART:
        return buildMultipart(call, substitute);
    case BodyFormat::FORM:
        return encodeForm(call.body, substitute);
    case BodyFormat::JSON:
        return buildJson(call.body, substitute);
    case BodyFormat::XML:
    case BodyFormat::TEXT:
        return asText(call.body, substitute);
    case BodyFormat::NONE:
    default: {
        BuiltBody b;
        b.contentType = QString();
        b.body = QByteArray();
        b.snapshot = QString();
        return b;
    }
    }
}

} // namespace rcm
