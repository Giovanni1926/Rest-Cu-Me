#pragma once

#include <QString>
#include <optional>

namespace rcm {

struct BodyFormat {
    enum Value { NONE, JSON, XML, TEXT, FORM, MULTIPART };

    Value value = NONE;

    BodyFormat() = default;
    BodyFormat(Value v) : value(v) {}

    QString id() const {
        switch (value) {
        case JSON: return QStringLiteral("json");
        case XML: return QStringLiteral("xml");
        case TEXT: return QStringLiteral("text");
        case FORM: return QStringLiteral("form");
        case MULTIPART: return QStringLiteral("multipart");
        case NONE: default: return QStringLiteral("none");
        }
    }

    QString defaultContentType() const {
        switch (value) {
        case JSON: return QStringLiteral("application/json");
        case XML: return QStringLiteral("application/xml");
        case TEXT: return QStringLiteral("text/plain");
        case FORM: return QStringLiteral("application/x-www-form-urlencoded");
        case MULTIPART: return QStringLiteral("multipart/form-data");
        case NONE: default: return QString();
        }
    }

    static std::optional<BodyFormat> fromId(const QString& id) {
        const QString trimmed = id.trimmed();
        for (int v = NONE; v <= MULTIPART; ++v) {
            BodyFormat f(static_cast<Value>(v));
            if (f.id().compare(trimmed, Qt::CaseInsensitive) == 0) return f;
        }
        return std::nullopt;
    }

    static BodyFormat infer(const QString& bodyFormat, const QString& contentType, bool hasBody) {
        auto explicitFormat = fromId(bodyFormat);
        if (explicitFormat) return *explicitFormat;
        const QString type = contentType.toLower();
        if (type.contains(QStringLiteral("multipart"))) return BodyFormat(MULTIPART);
        if (type.contains(QStringLiteral("x-www-form-urlencoded"))) return BodyFormat(FORM);
        if (type.contains(QStringLiteral("json"))) return BodyFormat(JSON);
        if (type.contains(QStringLiteral("xml"))) return BodyFormat(XML);
        return hasBody ? BodyFormat(TEXT) : BodyFormat(NONE);
    }
};

} // namespace rcm
