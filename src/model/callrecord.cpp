#include "callrecord.h"
#include <QDateTime>

namespace rcm {

QString CallRecord::mapToString(const VarMap& map) {
    QStringList parts;
    for (const auto& kv : map) {
        parts << escapeValue(kv.first) + QStringLiteral(":") + escapeValue(kv.second);
    }
    return parts.join(QStringLiteral(","));
}

int CallRecord::findUnescaped(const QString& s, QChar ch, int from) {
    int i = from;
    while (i < s.size()) {
        if (s.at(i) == QLatin1Char('\\')) {
            i += 2;
            continue;
        }
        if (s.at(i) == ch) return i;
        ++i;
    }
    return -1;
}

VarMap CallRecord::parseMap(const QString& s) {
    VarMap result;
    if (s.isEmpty() || s.trimmed().isEmpty()) return result;
    int start = 0;
    while (start < s.size()) {
        int colon = findUnescaped(s, QLatin1Char(':'), start);
        if (colon < 0) break;
        QString key = unescapeValue(s.mid(start, colon - start));
        int comma = findUnescaped(s, QLatin1Char(','), colon + 1);
        if (comma < 0) {
            result.emplace_back(key, unescapeValue(s.mid(colon + 1)));
            break;
        }
        result.emplace_back(key, unescapeValue(s.mid(colon + 1, comma - colon - 1)));
        start = comma + 1;
    }
    return result;
}

QString CallRecord::serialize() const {
    QStringList lines;
    lines << QStringLiteral("id=") + id;
    lines << QStringLiteral("timestamp=") + timestamp;
    lines << QStringLiteral("method=") + method;
    lines << QStringLiteral("url=") + url;
    lines << QStringLiteral("label=") + escapeValue(label);
    lines << QStringLiteral("requestHeaders=") + mapToString(requestHeaders);
    lines << QStringLiteral("requestCookies=") + mapToString(requestCookies);
    lines << QStringLiteral("requestBody=") + escapeValue(requestBody);
    lines << QStringLiteral("statusCode=") + QString::number(statusCode);
    lines << QStringLiteral("responseHeaders=") + mapToString(responseHeaders);
    lines << QStringLiteral("responseCookies=") + mapToString(responseCookies);
    lines << QStringLiteral("responseBody=") + escapeValue(responseBody);
    return lines.join(QStringLiteral("\n"));
}

CallRecord CallRecord::deserialize(const QString& text) {
    CallRecord record;
    const QStringList lines = text.split(QStringLiteral("\n"));
    for (const QString& line : lines) {
        const int eq = line.indexOf(QStringLiteral("="));
        if (eq < 0) continue;
        const QString key = line.left(eq);
        const QString value = line.mid(eq + 1);
        if (key == QStringLiteral("id")) record.id = value;
        else if (key == QStringLiteral("timestamp")) record.timestamp = value;
        else if (key == QStringLiteral("method")) record.method = value;
        else if (key == QStringLiteral("url")) record.url = value;
        else if (key == QStringLiteral("label")) record.label = unescapeValue(value);
        else if (key == QStringLiteral("requestHeaders")) record.requestHeaders = parseMap(value);
        else if (key == QStringLiteral("requestCookies")) record.requestCookies = parseMap(value);
        else if (key == QStringLiteral("requestBody")) record.requestBody = unescapeValue(value);
        else if (key == QStringLiteral("statusCode")) {
            bool ok = false;
            record.statusCode = value.trimmed().toInt(&ok);
            if (!ok) record.statusCode = 0;
        } else if (key == QStringLiteral("responseHeaders")) record.responseHeaders = parseMap(value);
        else if (key == QStringLiteral("responseCookies")) record.responseCookies = parseMap(value);
        else if (key == QStringLiteral("responseBody")) record.responseBody = unescapeValue(value);
    }
    record.timestamp = currentTimestamp();
    return record;
}

} // namespace rcm
