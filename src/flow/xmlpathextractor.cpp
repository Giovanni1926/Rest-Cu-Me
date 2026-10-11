#include "xmlpathextractor.h"

#include <pugixml.hpp>
#include "../util/str.h"

namespace rcm {

bool XmlPathExtractor::isXPath(const QString& expression) {
    if (expression.isNull()) return false;
    const QString expr = expression.trimmed();
    return expr.startsWith(QLatin1Char('/')) || expr.startsWith(QLatin1Char('(')) ||
           expr.startsWith(QLatin1Char('@')) || expr.startsWith(QStringLiteral("text("));
}

QString XmlPathExtractor::extract(const QString& expression, const QString& xml) {
    if (expression.trimmed().isEmpty() || xml.trimmed().isEmpty()) return QString();
    try {
        pugi::xml_document doc;
        if (!doc.load_string(q2s(xml).c_str())) return QString();
        pugi::xpath_query query(q2s(expression.trimmed()).c_str());
        const std::string value = query.evaluate_string(doc);
        return s2q(value);
    } catch (...) {
        return QString();
    }
}

} // namespace rcm
