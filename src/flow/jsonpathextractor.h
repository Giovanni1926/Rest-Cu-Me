#pragma once

#include <QString>
#include "../http/restclient.h"

namespace rcm {

class JsonPathExtractor {
public:
    static QString extract(const QString& expression, const CallResult& result);

private:
    static std::vector<QString> tokenize(const QString& path);
    static void walk(const Json& el, const std::vector<QString>& tokens, int idx,
                     std::vector<Json>& out);
    static QString render(const Json& el);
};

} // namespace rcm
