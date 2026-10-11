#include "outputsuggestions.h"

#include <pugixml.hpp>
#include <queue>
#include <set>

namespace rcm {

static void addUnique(QStringList& list, std::set<QString>& seen, const QString& value) {
    if (seen.insert(value).second) list << value;
}

QStringList OutputSuggestions::fromResult(const CallResult* result) {
    QStringList list;
    std::set<QString> seen;
    addUnique(list, seen, QStringLiteral("$"));
    addUnique(list, seen, QStringLiteral("statusCode"));

    if (!result) return list;

    for (const auto& kv : result->responseHeaders) {
        addUnique(list, seen, QStringLiteral("headers.") + kv.first);
    }

    const QString body = result->responseBody;
    const QString stripped = body;
    if (stripped.trimmed().startsWith(QLatin1Char('<'))) {
        try {
            pugi::xml_document doc;
            if (doc.load_string(q2s(body).c_str())) {
                std::queue<pugi::xml_node> queue;
                queue.push(doc.document_element());
                int visited = 0;
                std::set<std::string> names;
                while (!queue.empty() && visited < 30) {
                    pugi::xml_node node = queue.front();
                    queue.pop();
                    ++visited;
                    const std::string name = node.name();
                    if (names.insert(name).second) {
                        QString expr = QStringLiteral("//*[local-name()='") + s2q(name) +
                                       QStringLiteral("']");
                        bool hasElementChild = false;
                        for (pugi::xml_node c = node.first_child(); c; c = c.next_sibling()) {
                            if (c.type() == pugi::node_element) {
                                hasElementChild = true;
                                break;
                            }
                        }
                        if (!hasElementChild) expr += QStringLiteral("/text()");
                        addUnique(list, seen, expr);
                    }
                    for (pugi::xml_node c = node.first_child(); c; c = c.next_sibling()) {
                        if (c.type() == pugi::node_element) queue.push(c);
                    }
                }
            }
        } catch (...) {
        }
    } else {
        try {
            Json parsed = Json::parse(q2s(body));
            if (parsed.is_object()) {
                for (auto it = parsed.begin(); it != parsed.end(); ++it) {
                    addUnique(list, seen, QStringLiteral("$.") + s2q(it.key()));
                }
            } else if (parsed.is_array() && !parsed.empty()) {
                addUnique(list, seen, QStringLiteral("$[0]"));
                const Json& first = parsed.at(0);
                if (first.is_object()) {
                    for (auto it = first.begin(); it != first.end(); ++it) {
                        addUnique(list, seen, QStringLiteral("$[0].") + s2q(it.key()));
                    }
                }
            }
        } catch (...) {
        }
    }

    return list;
}

} // namespace rcm
