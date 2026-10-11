#pragma once

#include <QString>
#include "../util/str.h"
#include "../model/restcalldef.h"
#include "../model/rcmvariables.h"

namespace rcm {

struct CallResult {
    QString method;
    QString url;
    int statusCode = 0;
    QString statusText;
    QString responseBody;
    long long latencyMs = 0;
    bool success = false;
    QString errorMessage;
    QString requestBody;
    VarMap requestHeaders;
    VarMap requestCookies;
    VarMap responseHeaders;
    VarMap responseCookies;

    CallResult() = default;

    CallResult(const QString& method, const QString& url, int statusCode,
               const QString& statusText, const QString& responseBody, long long latencyMs,
               bool success, const QString& errorMessage)
        : method(method), url(url), statusCode(statusCode), statusText(statusText),
          responseBody(responseBody), latencyMs(latencyMs), success(success),
          errorMessage(errorMessage) {}

    QString summary() const {
        if (!errorMessage.isEmpty()) {
            return method + QStringLiteral(" ") + url + QStringLiteral(" \u2192 ERROR: ") + errorMessage;
        }
        return method + QStringLiteral(" ") + url + QStringLiteral(" \u2192 ") +
               QString::number(statusCode) + QStringLiteral(" ") + statusText +
               QStringLiteral(" (") + QString::number(latencyMs) + QStringLiteral("ms)");
    }

    QString detailed() const;
};

class RestClient {
public:
    explicit RestClient(const RcmVariables& variables) : variables_(variables) {}

    CallResult execute(const RestCallDef& call);
    CallResult executeRaw(const QString& method, const QString& url, const VarMap& headers,
                          const VarMap& cookies, const QString& body);

    static QString statusText(int code);
    static QString appendQueryParams(const QString& url, const VarMap& queryParams);
    static QString urlEncode(const QString& s);

private:
    QString substitute(const QString& input) const;
    VarMap substituteHeaders(const VarMap& headers) const;
    HttpOptions substituteOptions(const HttpOptions& options) const;
    void applyAuth(const AuthDef& auth, VarMap& headers, VarMap& queryParams) const;
    static QString buildCookieHeader(const VarMap& cookies);

    RcmVariables variables_;
};

} // namespace rcm
