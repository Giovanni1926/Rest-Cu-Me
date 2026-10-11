#include "restcallparser.h"
#include "rcmconstants.h"

namespace rcm {

static QString strOrNull(const Json& o, const char* key) {
    if (!o.is_object()) return QString();
    auto it = o.find(key);
    if (it == o.end() || it->is_null()) return QString();
    if (it->is_string()) return s2q(it->get<std::string>());
    return s2q(it->dump());
}

static QString requiredString(const Json& o, const char* key) {
    auto it = o.find(key);
    if (it == o.end() || !it->is_string()) return QString();
    return s2q(it->get<std::string>());
}

int RestCallParser::intField(const Json& parent, const char* objectName, const char* field) {
    if (!parent.is_object()) return 0;
    auto it = parent.find(objectName);
    if (it == parent.end() || !it->is_object()) return 0;
    auto f = it->find(field);
    if (f == it->end() || f->is_null()) return 0;
    try {
        int v = f->get<int>();
        return v > 0 ? v : 0;
    } catch (...) {
        return 0;
    }
}

TlsConfig RestCallParser::parseTls(const Json& json) {
    if (!json.is_object()) return TlsConfig::defaults();
    auto it = json.find("tls");
    if (it == json.end() || !it->is_object()) return TlsConfig::defaults();
    return TlsConfig(strOrNull(*it, "verification"), strOrNull(*it, "caCert"),
                     strOrNull(*it, "clientCert"), strOrNull(*it, "clientKey"),
                     strOrNull(*it, "keyPassword"));
}

HttpOptions RestCallParser::parseHttpOptions(const Json& json) {
    if (!json.is_object()) return HttpOptions::defaults();
    return HttpOptions(intField(json, "timeout", "connect"),
                       intField(json, "timeout", "read"), parseTls(json));
}

std::optional<AuthDef> RestCallParser::parseAuth(const Json& authObj, std::vector<QString>& errors) {
    if (!authObj.is_object()) {
        errors.push_back(QStringLiteral("Auth object must have a 'type' field"));
        return std::nullopt;
    }
    if (authObj.find("type") == authObj.end()) {
        errors.push_back(QStringLiteral("Auth object must have a 'type' field"));
        return std::nullopt;
    }
    const QString type = requiredString(authObj, "type").toLower();
    if (!validAuthTypes().count(type)) {
        errors.push_back(QStringLiteral("Invalid auth type: ") + type +
                         QStringLiteral(". Must be one of: ") + authTypesToString());
        return std::nullopt;
    }
    if (type == QStringLiteral("none")) return AuthDef::none();
    if (type == QStringLiteral("bearer")) {
        if (authObj.find("token") == authObj.end()) {
            errors.push_back(QStringLiteral("Bearer auth requires 'token' field"));
            return std::nullopt;
        }
        return AuthDef::bearer(requiredString(authObj, "token"));
    }
    if (type == QStringLiteral("basic")) {
        if (authObj.find("username") == authObj.end() || authObj.find("password") == authObj.end()) {
            errors.push_back(QStringLiteral("Basic auth requires 'username' and 'password' fields"));
            return std::nullopt;
        }
        return AuthDef::basic(requiredString(authObj, "username"),
                              requiredString(authObj, "password"));
    }
    if (type == QStringLiteral("apikey")) {
        if (authObj.find("key") == authObj.end() || authObj.find("value") == authObj.end() ||
            authObj.find("in") == authObj.end()) {
            errors.push_back(QStringLiteral("API key auth requires 'key', 'value', and 'in' fields"));
            return std::nullopt;
        }
        const QString in = requiredString(authObj, "in");
        if (!validApiKeyIn().count(in)) {
            errors.push_back(QStringLiteral("API key 'in' must be one of: ") + apiKeyInToString());
            return std::nullopt;
        }
        return AuthDef::apiKey(requiredString(authObj, "key"),
                               requiredString(authObj, "value"), in);
    }
    return std::nullopt;
}

std::vector<MultipartPart> RestCallParser::parseMultipart(const Json& json) {
    std::vector<MultipartPart> parts;
    if (!json.is_object()) return parts;
    auto it = json.find("multipart");
    if (it == json.end() || !it->is_array()) return parts;
    for (const auto& item : *it) {
        if (!item.is_object()) continue;
        parts.push_back(MultipartPart::fromJson(item));
    }
    return parts;
}

std::optional<RestCallDef> RestCallParser::parseCallDefinition(const Json& json,
                                                               std::vector<QString>& errors) {
    if (!json.is_object()) return std::nullopt;
    for (const QString& f : requiredCallFields()) {
        if (json.find(q2s(f)) == json.end()) {
            errors.push_back(QStringLiteral("Missing required field '") + f +
                             QStringLiteral("' in call definition"));
            return std::nullopt;
        }
    }

    const QString method = requiredString(json, "method").toUpper();
    if (!validMethods().count(method)) {
        errors.push_back(QStringLiteral("Invalid HTTP method: ") + method +
                         QStringLiteral(". Must be one of: ") + methodsToString());
        return std::nullopt;
    }

    const QString url = requiredString(json, "url");

    VarMap queryParams;
    if (json.find("query") != json.end() && json.at("query").is_object()) {
        queryParams = fromJsonObject(json.at("query"));
    }

    VarMap headers;
    if (json.find("headers") != json.end() && json.at("headers").is_object()) {
        headers = fromJsonObject(json.at("headers"));
    }

    VarMap cookies;
    if (json.find("cookies") != json.end() && json.at("cookies").is_object()) {
        cookies = fromJsonObject(json.at("cookies"));
    }

    auto auth = parseAuth(json.at("auth"), errors);
    if (!auth) return std::nullopt;

    Json body;
    if (json.at("body").is_object()) {
        body = json.at("body");
    } else if (json.at("body").is_string()) {
        body = json.at("body").get<std::string>();
    } else {
        body = std::string("");
    }

    const QString contentType = requiredString(json, "contentType");

    bool hasParts = json.find("multipart") != json.end() && json.at("multipart").is_array() &&
                    !json.at("multipart").empty();
    bool hasBody = (!body.is_null() && !s2q(body.is_string() ? body.get<std::string>() : body.dump()).isEmpty()) || hasParts;
    const BodyFormat format = BodyFormat::infer(strOrNull(json, "bodyFormat"), contentType, hasBody);

    std::vector<MultipartPart> multipart = parseMultipart(json);
    HttpOptions options = parseHttpOptions(json);

    return RestCallDef(method, url, queryParams, headers, cookies, *auth, body, contentType,
                       options, format, multipart);
}

} // namespace rcm
