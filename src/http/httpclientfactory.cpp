#include "httpclientfactory.h"

#include <QSslConfiguration>
#include <QSslCertificate>
#include <QSslKey>
#include <QFile>
#include <QByteArray>

namespace rcm {

std::optional<QSslConfiguration> HttpClientFactory::buildSslConfiguration(const TlsConfig& tls) {
    TlsConfig effective = tls.verification.isEmpty() ? TlsConfig::defaults() : tls;

    bool hasCustomTrust = false;
    QList<QSslCertificate> caCerts;
    bool trustAll = effective.isTrustAll();
    bool hasClient = effective.hasClientCert();

    if (effective.isCustom() && !effective.caCert.isEmpty()) {
        hasCustomTrust = true;
        caCerts = QSslCertificate::fromPath(effective.caCert);
    }

    if (!trustAll && !hasCustomTrust && !hasClient) {
        return std::nullopt;
    }

    QSslConfiguration config = QSslConfiguration::defaultConfiguration();

    if (trustAll) {
        config.setPeerVerifyMode(QSslSocket::VerifyNone);
    }

    if (hasCustomTrust) {
        config.addCaCertificates(caCerts);
    }

    if (hasClient) {
        QList<QSslCertificate> clientCerts = QSslCertificate::fromPath(effective.clientCert);
        if (!clientCerts.isEmpty()) {
            config.setLocalCertificateChain(clientCerts);
        }
        QByteArray keyBytes = QFile(effective.clientKey).readAll();
        const QByteArray pass = effective.keyPassword.isEmpty()
                                    ? QByteArray()
                                    : effective.keyPassword.toUtf8();
        for (QSsl::KeyAlgorithm alg : {QSsl::Rsa, QSsl::Ec, QSsl::Dsa}) {
            QSslKey candidate(keyBytes, alg, QSsl::Pem, QSsl::PrivateKey, pass);
            if (!candidate.isNull()) {
                config.setPrivateKey(candidate);
                break;
            }
        }
    }

    return config;
}

} // namespace rcm
