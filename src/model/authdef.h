#pragma once

#include <QString>

namespace rcm {

struct AuthDef {
    enum Type { NONE, BEARER, BASIC, APIKEY };

    Type type = NONE;
    QString token;
    QString username;
    QString password;
    QString key;
    QString value;
    QString in;

    static AuthDef none() { return AuthDef(); }
    static AuthDef bearer(const QString& token) {
        AuthDef a;
        a.type = BEARER;
        a.token = token;
        return a;
    }
    static AuthDef basic(const QString& username, const QString& password) {
        AuthDef a;
        a.type = BASIC;
        a.username = username;
        a.password = password;
        return a;
    }
    static AuthDef apiKey(const QString& key, const QString& value, const QString& in) {
        AuthDef a;
        a.type = APIKEY;
        a.key = key;
        a.value = value;
        a.in = in;
        return a;
    }

    QString typeName() const {
        switch (type) {
        case BEARER: return QStringLiteral("bearer");
        case BASIC: return QStringLiteral("basic");
        case APIKEY: return QStringLiteral("apikey");
        case NONE: default: return QStringLiteral("none");
        }
    }
};

} // namespace rcm
