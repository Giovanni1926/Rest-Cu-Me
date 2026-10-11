#pragma once

#include <QString>

namespace rcm {

struct TlsConfig {
    static inline const QString DEFAULT = QStringLiteral("default");
    static inline const QString TRUST_ALL = QStringLiteral("trustAll");
    static inline const QString CUSTOM = QStringLiteral("custom");

    QString verification;
    QString caCert;
    QString clientCert;
    QString clientKey;
    QString keyPassword;

    TlsConfig() = default;
    TlsConfig(const QString& verification, const QString& caCert, const QString& clientCert,
              const QString& clientKey, const QString& keyPassword)
        : verification(normalizeVerification(verification)),
          caCert(blankToEmpty(caCert)), clientCert(blankToEmpty(clientCert)),
          clientKey(blankToEmpty(clientKey)),
          keyPassword(clientKey.isEmpty() ? QString() : keyPassword) {}

    static QString normalizeVerification(const QString& v) {
        return (v.isEmpty() || v.trimmed().isEmpty()) ? DEFAULT : v;
    }

    static QString blankToEmpty(const QString& s) {
        return (s.isNull() || s.trimmed().isEmpty()) ? QString() : s;
    }

    static TlsConfig defaults() { return TlsConfig(DEFAULT, QString(), QString(), QString(), QString()); }

    bool isTrustAll() const { return verification == TRUST_ALL; }
    bool isCustom() const { return verification == CUSTOM; }
    bool hasClientCert() const { return !clientCert.isEmpty() && !clientKey.isEmpty(); }
};

} // namespace rcm
