#pragma once

#include <QString>
#include <vector>
#include <utility>
#include <json.hpp>

using Json = nlohmann::ordered_json;

using VarMap = std::vector<std::pair<QString, QString>>;

inline QString s2q(const std::string& s) {
    return QString::fromUtf8(s.data(), static_cast<int>(s.size()));
}

inline std::string q2s(const QString& s) {
    return s.toUtf8().toStdString();
}

inline QString firstValue(const VarMap& map, const QString& name) {
    for (const auto& kv : map) {
        if (kv.first == name) return kv.second;
    }
    return QString();
}

inline void put(VarMap& map, const QString& key, const QString& value) {
    for (auto& kv : map) {
        if (kv.first == key) {
            kv.second = value;
            return;
        }
    }
    map.emplace_back(key, value);
}

inline VarMap fromJsonObject(const Json& obj) {
    VarMap result;
    if (obj.is_null()) return result;
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        QString v;
        if (it.value().is_string()) v = s2q(it.value().get<std::string>());
        else if (it.value().is_primitive()) {
            if (it.value().is_null()) v.clear();
            else v = s2q(it.value().dump());
        }
        result.emplace_back(s2q(it.key()), v);
    }
    return result;
}

inline Json toJsonObject(const VarMap& map) {
    Json obj = Json::object();
    for (const auto& kv : map) {
        obj[q2s(kv.first)] = q2s(kv.second);
    }
    return obj;
}

inline QString escapeValue(const QString& s) {
    QString r = s;
    r.replace(QStringLiteral("\\"), QStringLiteral("\\\\"));
    r.replace(QStringLiteral("\n"), QStringLiteral("\\n"));
    r.replace(QStringLiteral("\r"), QStringLiteral("\\r"));
    r.replace(QStringLiteral("="), QStringLiteral("\\="));
    return r;
}

inline QString unescapeValue(const QString& s) {
    QString r = s;
    r.replace(QStringLiteral("\\n"), QStringLiteral("\n"));
    r.replace(QStringLiteral("\\r"), QStringLiteral("\r"));
    r.replace(QStringLiteral("\\="), QStringLiteral("="));
    r.replace(QStringLiteral("\\\\"), QStringLiteral("\\"));
    return r;
}
