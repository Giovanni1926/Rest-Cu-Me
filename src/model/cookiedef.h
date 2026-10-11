#pragma once

#include <QString>
#include <QStringList>
#include "../util/str.h"

namespace rcm {

class CookieDef {
public:
    CookieDef() = default;
    explicit CookieDef(const VarMap& cookies) : cookies_(cookies) {}

    void addCookie(const QString& name, const QString& value) { put(cookies_, name, value); }
    QString getCookie(const QString& name) const { return firstValue(cookies_, name); }
    VarMap allCookies() const { return cookies_; }

    QString toRequestHeader() const {
        QString sb;
        for (const auto& kv : cookies_) {
            if (!sb.isEmpty()) sb += QStringLiteral("; ");
            sb += kv.first + QStringLiteral("=") + kv.second;
        }
        return sb;
    }

    static CookieDef fromSetCookieHeader(const QString& headerValue) {
        CookieDef c;
        if (headerValue.isEmpty() || headerValue.trimmed().isEmpty()) return c;
        const QStringList parts = headerValue.split(QStringLiteral(";"));
        if (parts.isEmpty()) return c;
        const QString first = parts.first().trimmed();
        const int eq = first.indexOf(QStringLiteral("="));
        if (eq < 0) return c;
        c.addCookie(first.left(eq).trimmed(), first.mid(eq + 1).trimmed());
        return c;
    }

    static CookieDef fromRequestHeader(const QString& headerValue) {
        CookieDef c;
        if (headerValue.isEmpty() || headerValue.trimmed().isEmpty()) return c;
        const QStringList parts = headerValue.split(QStringLiteral(";"));
        for (const QString& part : parts) {
            const int eq = part.indexOf(QStringLiteral("="));
            if (eq < 0) continue;
            c.addCookie(part.left(eq).trimmed(), part.mid(eq + 1).trimmed());
        }
        return c;
    }

    QString toString() const { return toRequestHeader(); }
    bool isEmpty() const { return cookies_.empty(); }
    int size() const { return static_cast<int>(cookies_.size()); }

private:
    VarMap cookies_;
};

} // namespace rcm
