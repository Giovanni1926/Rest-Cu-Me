#pragma once

#include <QString>
#include <set>

namespace rcm {

inline const QString DEFAULT_USER_AGENT = QStringLiteral("Rest-Cu-Me/1.0");
inline const int HTTP_CONNECT_TIMEOUT_SEC = 30;
inline const int HTTP_READ_TIMEOUT_SEC = 60;
inline const int MAX_CONSOLE_BODY_LENGTH = 500;

inline const std::set<QString>& validMethods() {
    static const std::set<QString> s = {QStringLiteral("GET"), QStringLiteral("POST"),
                                        QStringLiteral("PUT"), QStringLiteral("PATCH"),
                                        QStringLiteral("DELETE")};
    return s;
}

inline const std::set<QString>& validAuthTypes() {
    static const std::set<QString> s = {QStringLiteral("none"), QStringLiteral("bearer"),
                                        QStringLiteral("basic"), QStringLiteral("apikey")};
    return s;
}

inline const std::set<QString>& validApiKeyIn() {
    static const std::set<QString> s = {QStringLiteral("header"), QStringLiteral("query")};
    return s;
}

inline const std::vector<QString>& requiredCallFields() {
    static const std::vector<QString> v = {QStringLiteral("method"), QStringLiteral("url"),
                                           QStringLiteral("headers"), QStringLiteral("auth"),
                                           QStringLiteral("body"), QStringLiteral("contentType")};
    return v;
}

inline QString methodsToString() {
    return QStringLiteral("[GET, POST, PUT, PATCH, DELETE]");
}

inline QString authTypesToString() {
    return QStringLiteral("[none, bearer, basic, apikey]");
}

inline QString apiKeyInToString() {
    return QStringLiteral("[header, query]");
}

} // namespace rcm
