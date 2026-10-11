#pragma once

#include <QStringList>
#include "../../http/restclient.h"

namespace rcm {

class OutputSuggestions {
public:
    static QStringList fromResult(const CallResult* result);
};

} // namespace rcm
