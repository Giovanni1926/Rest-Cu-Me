#pragma once

#include <QString>
#include <QDateTime>
#include "../util/str.h"

namespace rcm {

class CallRecord {
public:
    QString id;
    QString timestamp;
    QString method;
    QString url;
    QString label;
    VarMap requestHeaders;
    VarMap requestCookies;
    QString requestBody;
    int statusCode = 0;
    VarMap responseHeaders;
    VarMap responseCookies;
    QString responseBody;

    CallRecord() { timestamp = currentTimestamp(); }

    CallRecord(const QString& id, const QString& method, const QString& url, const QString& label,
               const VarMap& requestHeaders, const VarMap& requestCookies,
               const QString& requestBody, int statusCode,
               const VarMap& responseHeaders, const VarMap& responseCookies,
               const QString& responseBody)
        : id(id), method(orEmpty(method)), url(orEmpty(url)), label(orEmpty(label)),
          requestHeaders(requestHeaders), requestCookies(requestCookies),
          requestBody(orEmpty(requestBody)), statusCode(statusCode),
          responseHeaders(responseHeaders), responseCookies(responseCookies),
          responseBody(orEmpty(responseBody)) {
        timestamp = currentTimestamp();
    }

    static QString orEmpty(const QString& s) { return s.isNull() ? QString() : s; }

    bool isSuccess() const { return statusCode >= 200 && statusCode < 300; }

    QString toString() const {
        return method + QStringLiteral(" ") + url + QStringLiteral(" -> ") +
               QString::number(statusCode) + QStringLiteral(" [") + timestamp +
               QStringLiteral("]");
    }

    static QString currentTimestamp() {
        return QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    }

    QString serialize() const;

    static CallRecord deserialize(const QString& text);

private:
    static QString mapToString(const VarMap& map);
    static VarMap parseMap(const QString& s);
    static int findUnescaped(const QString& s, QChar ch, int from);
};

} // namespace rcm
