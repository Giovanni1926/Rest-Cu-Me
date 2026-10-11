#pragma once

#include <QString>
#include <optional>
#include "../model/tlsconfig.h"

class QSslConfiguration;

namespace rcm {

class HttpClientFactory {
public:
    static std::optional<QSslConfiguration> buildSslConfiguration(const TlsConfig& tls);
};

} // namespace rcm
