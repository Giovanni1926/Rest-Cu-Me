#pragma once

#include <QString>
#include "../util/str.h"

namespace rcm {

struct MultipartPart {
    static inline const QString TYPE_TEXT = QStringLiteral("text");
    static inline const QString TYPE_FILE = QStringLiteral("file");

    QString name;
    QString type;
    QString value;
    QString path;
    QString fileName;
    QString contentType;

    MultipartPart() = default;
    MultipartPart(const QString& name, const QString& type, const QString& value,
                  const QString& path, const QString& fileName, const QString& contentType)
        : name(name), type(normalizeType(type)), value(value.isEmpty() ? QString() : value),
          path(path.isEmpty() ? QString() : path),
          fileName(fileName.isEmpty() ? QString() : fileName),
          contentType(contentType.isEmpty() ? QString() : contentType) {}

    static QString normalizeType(const QString& t) {
        return t.compare(TYPE_FILE, Qt::CaseInsensitive) == 0 ? TYPE_FILE : TYPE_TEXT;
    }

    bool isFile() const { return type == TYPE_FILE; }

    Json toJson() const {
        Json o = Json::object();
        o["name"] = q2s(name);
        o["type"] = q2s(type);
        if (isFile()) {
            o["path"] = q2s(path);
            if (!fileName.isEmpty()) o["fileName"] = q2s(fileName);
        } else {
            o["value"] = q2s(value);
        }
        if (!contentType.isEmpty()) o["contentType"] = q2s(contentType);
        return o;
    }

    static QString str(const Json& o, const char* key) {
        if (!o.is_object()) return QString();
        auto it = o.find(key);
        if (it == o.end() || it->is_null()) return QString();
        if (it->is_string()) return s2q(it->get<std::string>());
        return QString();
    }

    static MultipartPart fromJson(const Json& o) {
        return MultipartPart(str(o, "name"), str(o, "type"), str(o, "value"),
                             str(o, "path"), str(o, "fileName"), str(o, "contentType"));
    }
};

} // namespace rcm
