#pragma once

#include <QString>
#include <optional>
#include <vector>
#include "../util/str.h"
#include "restcalldef.h"

namespace rcm {

class RestCallParser {
public:
    static std::optional<RestCallDef> parseCallDefinition(const Json& json,
                                                          std::vector<QString>& errors);

private:
    static std::optional<AuthDef> parseAuth(const Json& authObj, std::vector<QString>& errors);
    static HttpOptions parseHttpOptions(const Json& json);
    static TlsConfig parseTls(const Json& json);
    static int intField(const Json& parent, const char* objectName, const char* field);
    static std::vector<MultipartPart> parseMultipart(const Json& json);
};

} // namespace rcm
