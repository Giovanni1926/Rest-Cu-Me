#include "restclient.h"
#include "httpclientfactory.h"
#include "requestbodybuilder.h"
#include "../model/rcmconstants.h"
#include "../model/cookiedef.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QUrl>
#include <QBuffer>
#include <QTimer>
#include <QEventLoop>
#include <QElapsedTimer>
#include <cctype>

namespace rcm {

QString CallResult::detailed() const {
    QString result = summary();
    if (!responseBody.isEmpty()) {
        result += QStringLiteral("\n");
        try {
            Json parsed = Json::parse(q2s(responseBody));
            result += s2q(parsed.dump(2));
        } catch (...) {
            result += responseBody;
        }
    }
    return result;
}

QString RestClient::urlEncode(const QString& s) {
    const QByteArray bytes = s.toUtf8();
    QString out;
    out.reserve(bytes.size() * 3);
    for (char c : bytes) {
        const unsigned char uc = static_cast<unsigned char>(c);
        if (std::isalnum(uc) || c == '-' || c == '_' || c == '.' || c == '*') {
            out += QLatin1Char(c);
        } else if (c == ' ') {
            out += QLatin1Char('+');
        } else {
            out += QStringLiteral("%") + QString::number(uc, 16).rightJustified(2, QLatin1Char('0')).toUpper();
        }
    }
    return out;
}

QString RestClient::appendQueryParams(const QString& url, const VarMap& queryParams) {
    if (queryParams.empty()) return url;
    bool first = !url.contains(QLatin1Char('?'));
    QString result = url;
    for (const auto& kv : queryParams) {
        if (kv.first.isEmpty()) continue;
        result += first ? QStringLiteral("?") : QStringLiteral("&");
        first = false;
        result += urlEncode(kv.first) + QStringLiteral("=") +
                  urlEncode(kv.second.isEmpty() ? QString() : kv.second);
    }
    return result;
}

QString RestClient::substitute(const QString& input) const {
    if (input.isNull()) return QString();
    QString result;
    int i = 0;
    while (i < input.size()) {
        if (input.at(i) == QLatin1Char('$') && i + 1 < input.size() &&
            input.at(i + 1) == QLatin1Char('{')) {
            int close = input.indexOf(QLatin1Char('}'), i + 2);
            if (close > 0) {
                const QString varName = input.mid(i + 2, close - i - 2);
                if (variables_.hasVariable(varName)) {
                    result += variables_.resolve(varName);
                } else {
                    result += QStringLiteral("${") + varName + QStringLiteral("}");
                }
                i = close + 1;
                continue;
            }
        }
        result += input.at(i);
        ++i;
    }
    return result;
}

VarMap RestClient::substituteHeaders(const VarMap& headers) const {
    VarMap result;
    for (const auto& kv : headers) {
        result.emplace_back(substitute(kv.first), substitute(kv.second));
    }
    return result;
}

HttpOptions RestClient::substituteOptions(const HttpOptions& options) const {
    if (options.tls.verification.isEmpty() && options.connectTimeoutSec == 0 &&
        options.readTimeoutSec == 0) {
        return HttpOptions::defaults();
    }
    if (options.tls.verification.isEmpty()) return options;
    TlsConfig tls(options.tls.verification, substitute(options.tls.caCert),
                  substitute(options.tls.clientCert), substitute(options.tls.clientKey),
                  substitute(options.tls.keyPassword));
    return HttpOptions(options.connectTimeoutSec, options.readTimeoutSec, tls);
}

void RestClient::applyAuth(const AuthDef& auth, VarMap& headers, VarMap& queryParams) const {
    switch (auth.type) {
    case AuthDef::BEARER:
        put(headers, QStringLiteral("Authorization"),
            QStringLiteral("Bearer ") + substitute(auth.token));
        break;
    case AuthDef::BASIC: {
        const QString credentials = substitute(auth.username) + QStringLiteral(":") +
                                    substitute(auth.password);
        const QByteArray encoded = credentials.toUtf8().toBase64();
        put(headers, QStringLiteral("Authorization"),
            QStringLiteral("Basic ") + QString::fromUtf8(encoded));
        break;
    }
    case AuthDef::APIKEY: {
        const QString key = substitute(auth.key);
        const QString value = substitute(auth.value);
        if (auth.in == QStringLiteral("header")) {
            put(headers, key, value);
        } else if (auth.in == QStringLiteral("query")) {
            put(queryParams, key, value);
        }
        break;
    }
    case AuthDef::NONE:
    default:
        break;
    }
}

QString RestClient::buildCookieHeader(const VarMap& cookies) {
    if (cookies.empty()) return QString();
    QStringList parts;
    for (const auto& kv : cookies) {
        parts << kv.first + QStringLiteral("=") + kv.second;
    }
    return parts.join(QStringLiteral("; "));
}

struct RawResponse {
    int statusCode = 0;
    QString statusText;
    QByteArray body;
    long long latencyMs = 0;
    QString errorMessage;
    VarMap responseHeaders;
    VarMap responseCookies;
};

static RawResponse sendSync(const QUrl& url, const QString& method,
                            const QList<QPair<QByteArray, QByteArray>>& headers,
                            const QByteArray& bodyData,
                            const std::optional<QSslConfiguration>& ssl,
                            int connectTimeoutSec, int readTimeoutSec) {
    QNetworkAccessManager nam;
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    if (ssl) request.setSslConfiguration(*ssl);
    for (const auto& h : headers) {
        request.setRawHeader(h.first, h.second);
    }

    QBuffer bodyBuffer;
    bodyBuffer.setData(bodyData);

    RawResponse resp;
    QNetworkReply* reply = nam.sendCustomRequest(request, method.toUtf8(), &bodyBuffer);

    bool headersReceived = false;
    bool aborted = false;
    QElapsedTimer timer;
    timer.start();

    QTimer connectTimer;
    connectTimer.setSingleShot(true);
    QTimer readTimer;
    readTimer.setSingleShot(true);

    QObject::connect(reply, &QNetworkReply::sslErrors, reply,
                     [reply, ssl](const QList<QSslError>&) {
                         if (ssl && ssl->peerVerifyMode() == QSslSocket::VerifyNone) {
                             reply->ignoreSslErrors();
                         }
                     });

    QObject::connect(reply, &QNetworkReply::metaDataChanged, &nam, [&]() {
        headersReceived = true;
        connectTimer.stop();
        readTimer.start(readTimeoutSec * 1000);
    });

    QObject::connect(reply, &QNetworkReply::finished, &nam, [&]() {
        connectTimer.stop();
        readTimer.stop();
        resp.latencyMs = timer.elapsed();
        resp.statusCode =
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        resp.statusText =
            reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
        resp.body = reply->readAll();

        const auto pairs = reply->rawHeaderPairs();
        for (const auto& p : pairs) {
            const QString key = QString::fromUtf8(p.first);
            const QString value = QString::fromUtf8(p.second);
            if (key.compare(QStringLiteral("Set-Cookie"), Qt::CaseInsensitive) == 0) {
                CookieDef cd = CookieDef::fromSetCookieHeader(value);
                for (const auto& kv : cd.allCookies()) {
                    put(resp.responseCookies, kv.first, kv.second);
                }
                continue;
            }
            bool found = false;
            for (auto& kv : resp.responseHeaders) {
                if (kv.first.compare(key, Qt::CaseInsensitive) == 0) {
                    kv.second = kv.second + QStringLiteral(", ") + value;
                    found = true;
                    break;
                }
            }
            if (!found) resp.responseHeaders.emplace_back(key, value);
        }

        if (reply->error() != QNetworkReply::NoError) {
            resp.errorMessage = aborted ? resp.errorMessage : reply->errorString();
        }
        reply->deleteLater();
    });

    QObject::connect(&connectTimer, &QTimer::timeout, &nam, [&]() {
        if (!headersReceived) {
            aborted = true;
            resp.errorMessage = QStringLiteral("Connect timeout");
            reply->abort();
        }
    });
    QObject::connect(&readTimer, &QTimer::timeout, &nam, [&]() {
        if (headersReceived) {
            aborted = true;
            resp.errorMessage = QStringLiteral("Read timeout");
            reply->abort();
        }
    });

    connectTimer.start(connectTimeoutSec * 1000);

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    return resp;
}

CallResult RestClient::execute(const RestCallDef& call) {
    QString method = call.method;
    VarMap headers = substituteHeaders(call.headers);
    QString contentType = substitute(call.contentType);

    VarMap queryParams;
    for (const auto& kv : call.queryParams) {
        queryParams.emplace_back(substitute(kv.first), substitute(kv.second));
    }

    VarMap resolvedReqCookies;
    for (const auto& kv : call.cookies) {
        resolvedReqCookies.emplace_back(kv.first, substitute(kv.second));
    }

    QString cookieHeader = buildCookieHeader(resolvedReqCookies);
    if (!cookieHeader.isEmpty()) put(headers, QStringLiteral("Cookie"), cookieHeader);

    applyAuth(call.auth, headers, queryParams);

    bool hasUserAgent = false;
    for (const auto& kv : headers) {
        if (kv.first.compare(QStringLiteral("User-Agent"), Qt::CaseInsensitive) == 0) {
            hasUserAgent = true;
            break;
        }
    }
    if (!hasUserAgent) put(headers, QStringLiteral("User-Agent"), DEFAULT_USER_AGENT);

    QString url = appendQueryParams(substitute(call.url), queryParams);

    HttpOptions options = substituteOptions(call.options);

    QString bodyStr;
    VarMap reqHeadersSnapshot;

    try {
        RequestBodyBuilder::BuiltBody built = RequestBodyBuilder::build(
            call, [this](const QString& s) { return substitute(s); });

        if (!built.contentType.trimmed().isEmpty()) {
            contentType = built.contentType;
        } else if (contentType.trimmed().isEmpty()) {
            contentType = call.format.defaultContentType();
        }

        if (!contentType.isEmpty()) {
            bool hasCt = false;
            for (const auto& kv : headers) {
                if (kv.first.compare(QStringLiteral("Content-Type"), Qt::CaseInsensitive) == 0) {
                    hasCt = true;
                    break;
                }
            }
            if (!hasCt) put(headers, QStringLiteral("Content-Type"), contentType);
        }

        bodyStr = built.snapshot;
        reqHeadersSnapshot = headers;

        QList<QPair<QByteArray, QByteArray>> rawHeaders;
        for (const auto& kv : headers) {
            rawHeaders.append(qMakePair(kv.first.toUtf8(), kv.second.toUtf8()));
        }

        const QByteArray bodyBytes = built.body;

        RawResponse resp = sendSync(QUrl(url), method, rawHeaders, bodyBytes,
                                    HttpClientFactory::buildSslConfiguration(options.tls),
                                    options.connectTimeoutSecOrDefault(),
                                    options.readTimeoutSecOrDefault());

        if (!resp.errorMessage.isEmpty()) {
            return CallResult(method, url, 0, QStringLiteral("Error"), QString(), resp.latencyMs,
                              false, resp.errorMessage);
        }

        const bool success = resp.statusCode >= 200 && resp.statusCode < 300;
        CallResult result(method, url, resp.statusCode, resp.statusText,
                          QString::fromUtf8(resp.body), resp.latencyMs, success, QString());
        result.requestBody = bodyStr;
        result.requestHeaders = reqHeadersSnapshot;
        result.requestCookies = resolvedReqCookies;
        result.responseHeaders = resp.responseHeaders;
        result.responseCookies = resp.responseCookies;
        return result;
    } catch (const std::exception& e) {
        CallResult result(method, url, 0, QStringLiteral("Error"), QString(), 0, false,
                          QString::fromLatin1(e.what()));
        result.requestBody = bodyStr;
        result.requestHeaders = reqHeadersSnapshot;
        result.requestCookies = resolvedReqCookies;
        return result;
    }
}

CallResult RestClient::executeRaw(const QString& method, const QString& url,
                                  const VarMap& headers, const VarMap& cookies,
                                  const QString& body) {
    VarMap headersCopy = headers;
    VarMap cookiesCopy = cookies;

    bool hasUserAgent = false;
    for (const auto& kv : headersCopy) {
        if (kv.first.compare(QStringLiteral("User-Agent"), Qt::CaseInsensitive) == 0) {
            hasUserAgent = true;
            break;
        }
    }
    if (!hasUserAgent) put(headersCopy, QStringLiteral("User-Agent"), DEFAULT_USER_AGENT);

    QString cookieHeader = buildCookieHeader(cookiesCopy);
    if (!cookieHeader.isEmpty()) put(headersCopy, QStringLiteral("Cookie"), cookieHeader);

    QList<QPair<QByteArray, QByteArray>> rawHeaders;
    for (const auto& kv : headersCopy) {
        rawHeaders.append(qMakePair(kv.first.toUtf8(), kv.second.toUtf8()));
    }

    const QString upper = method.toUpper();
    QString verb = upper;
    if (upper != QStringLiteral("GET") && upper != QStringLiteral("DELETE") &&
        upper != QStringLiteral("POST") && upper != QStringLiteral("PUT") &&
        upper != QStringLiteral("PATCH")) {
        verb = QStringLiteral("GET");
    }

    const QByteArray bodyBytes = (verb == QStringLiteral("GET") ||
                                  verb == QStringLiteral("DELETE") || body.isEmpty())
                                     ? QByteArray()
                                     : body.toUtf8();

    RawResponse resp = sendSync(QUrl(url), verb, rawHeaders, bodyBytes, std::nullopt,
                                HTTP_CONNECT_TIMEOUT_SEC, HTTP_READ_TIMEOUT_SEC);

    if (!resp.errorMessage.isEmpty()) {
        CallResult result(verb, url, 0, QStringLiteral("Error"), QString(), resp.latencyMs, false,
                          resp.errorMessage);
        result.requestBody = body;
        result.requestHeaders = headersCopy;
        result.requestCookies = cookiesCopy;
        return result;
    }

    const bool success = resp.statusCode >= 200 && resp.statusCode < 300;
    CallResult result(verb, url, resp.statusCode, resp.statusText, QString::fromUtf8(resp.body),
                      resp.latencyMs, success, QString());
    result.requestBody = body;
    result.requestHeaders = headersCopy;
    result.requestCookies = cookiesCopy;
    result.responseHeaders = resp.responseHeaders;
    result.responseCookies = resp.responseCookies;
    return result;
}

QString RestClient::statusText(int code) {
    switch (code) {
    case 200: return QStringLiteral("OK");
    case 201: return QStringLiteral("Created");
    case 202: return QStringLiteral("Accepted");
    case 204: return QStringLiteral("No Content");
    case 301: return QStringLiteral("Moved Permanently");
    case 302: return QStringLiteral("Found");
    case 304: return QStringLiteral("Not Modified");
    case 400: return QStringLiteral("Bad Request");
    case 401: return QStringLiteral("Unauthorized");
    case 403: return QStringLiteral("Forbidden");
    case 404: return QStringLiteral("Not Found");
    case 405: return QStringLiteral("Method Not Allowed");
    case 409: return QStringLiteral("Conflict");
    case 422: return QStringLiteral("Unprocessable Entity");
    case 429: return QStringLiteral("Too Many Requests");
    case 500: return QStringLiteral("Internal Server Error");
    case 502: return QStringLiteral("Bad Gateway");
    case 503: return QStringLiteral("Service Unavailable");
    case 504: return QStringLiteral("Gateway Timeout");
    default:
        if (code >= 200 && code < 300) return QStringLiteral("Success");
        if (code >= 400 && code < 500) return QStringLiteral("Client Error");
        if (code >= 500) return QStringLiteral("Server Error");
        return QStringLiteral("Unknown");
    }
}

} // namespace rcm
