#pragma once

#include <QString>
#include <vector>
#include "../util/str.h"
#include "authdef.h"
#include "bodyformat.h"
#include "httpoptions.h"
#include "multipartpart.h"

namespace rcm {

struct RestCallDef {
    QString method;
    QString url;
    VarMap queryParams;
    VarMap headers;
    VarMap cookies;
    AuthDef auth;
    Json body;
    QString contentType;
    HttpOptions options;
    BodyFormat format;
    std::vector<MultipartPart> multipart;

    RestCallDef() = default;

    RestCallDef(const QString& method, const QString& url, const VarMap& queryParams,
                const VarMap& headers, const VarMap& cookies, const AuthDef& auth,
                const Json& body, const QString& contentType, const HttpOptions& options,
                const BodyFormat& format, const std::vector<MultipartPart>& multipart)
        : method(method), url(url), queryParams(queryParams), headers(headers),
          cookies(cookies), auth(auth), body(body), contentType(contentType),
          options(options), format(format), multipart(multipart) {}

    bool hasBody() const {
        return (!body.is_null() && !bodyString().isEmpty()) || !multipart.empty();
    }

    QString bodyString() const {
        if (body.is_null()) return QString();
        if (body.is_string()) return s2q(body.get<std::string>());
        if (body.is_boolean()) return body.get<bool>() ? QStringLiteral("true") : QStringLiteral("false");
        if (body.is_number()) return s2q(body.dump());
        return s2q(body.dump());
    }
};

} // namespace rcm
