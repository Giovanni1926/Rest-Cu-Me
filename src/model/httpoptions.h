#pragma once

#include "tlsconfig.h"
#include "rcmconstants.h"

namespace rcm {

struct HttpOptions {
    int connectTimeoutSec = 0;
    int readTimeoutSec = 0;
    TlsConfig tls;

    HttpOptions() = default;
    HttpOptions(int connectTimeoutSec, int readTimeoutSec, const TlsConfig& tls)
        : connectTimeoutSec(connectTimeoutSec), readTimeoutSec(readTimeoutSec), tls(tls) {}

    static HttpOptions defaults() { return HttpOptions(0, 0, TlsConfig::defaults()); }

    int connectTimeoutSecOrDefault() const {
        return (connectTimeoutSec <= 0) ? HTTP_CONNECT_TIMEOUT_SEC : connectTimeoutSec;
    }
    int readTimeoutSecOrDefault() const {
        return (readTimeoutSec <= 0) ? HTTP_READ_TIMEOUT_SEC : readTimeoutSec;
    }
};

} // namespace rcm
