#pragma once

#include <QString>

namespace rcm {

class XmlPathExtractor {
public:
    static bool isXPath(const QString& expression);
    static QString extract(const QString& expression, const QString& xml);
};

} // namespace rcm
