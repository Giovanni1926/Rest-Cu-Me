#include "jsonpathextractor.h"
#include "xmlpathextractor.h"

namespace rcm {

QString JsonPathExtractor::render(const Json& el) {
    if (el.is_null()) return QStringLiteral("null");
    if (el.is_string()) return s2q(el.get<std::string>());
    return s2q(el.dump());
}

std::vector<QString> JsonPathExtractor::tokenize(const QString& path) {
    std::vector<QString> tokens;
    QString current;
    for (int i = 0; i < path.size(); ++i) {
        const QChar c = path.at(i);
        if (c == QLatin1Char('.')) {
            if (!current.isEmpty()) {
                tokens.push_back(current);
                current.clear();
            }
        } else if (c == QLatin1Char('[')) {
            if (!current.isEmpty()) {
                tokens.push_back(current);
                current.clear();
            }
            int close = path.indexOf(QLatin1Char(']'), i);
            if (close < 0) {
                tokens.push_back(path.mid(i));
                return tokens;
            }
            tokens.push_back(path.mid(i, close - i + 1));
            i = close;
        } else {
            current += c;
        }
    }
    if (!current.isEmpty()) tokens.push_back(current);
    return tokens;
}

void JsonPathExtractor::walk(const Json& el, const std::vector<QString>& tokens, int idx,
                             std::vector<Json>& out) {
    if (idx >= static_cast<int>(tokens.size())) {
        out.push_back(el);
        return;
    }
    const QString token = tokens[idx];

    if (token.startsWith(QLatin1Char('['))) {
        const QString inner = token.mid(1, token.size() - 2);
        if (el.is_array()) {
            if (inner == QStringLiteral("*")) {
                for (const auto& item : el) {
                    walk(item, tokens, idx + 1, out);
                }
            } else {
                bool ok = false;
                const int index = inner.toInt(&ok);
                if (ok && index >= 0 && index < static_cast<int>(el.size())) {
                    walk(el.at(index), tokens, idx + 1, out);
                }
            }
        }
        return;
    }

    if (token == QStringLiteral("*") && el.is_object()) {
        for (auto it = el.begin(); it != el.end(); ++it) {
            walk(it.value(), tokens, idx + 1, out);
        }
        return;
    }

    if (el.is_object()) {
        auto it = el.find(q2s(token));
        if (it != el.end() && !it->is_null()) {
            walk(*it, tokens, idx + 1, out);
        }
    }
}

QString JsonPathExtractor::extract(const QString& expression, const CallResult& result) {
    const QString expr = expression.trimmed();

    if (expr.isEmpty() || expr == QStringLiteral("$")) {
        return result.responseBody;
    }
    if (expr.compare(QStringLiteral("statusCode"), Qt::CaseInsensitive) == 0) {
        return QString::number(result.statusCode);
    }
    if (expr.toLower().startsWith(QStringLiteral("headers."))) {
        const QString name = expr.mid(8);
        for (const auto& kv : result.responseHeaders) {
            if (kv.first.compare(name, Qt::CaseInsensitive) == 0) return kv.second;
        }
        return QString();
    }
    if (XmlPathExtractor::isXPath(expr)) {
        return XmlPathExtractor::extract(expr, result.responseBody);
    }

    Json root;
    try {
        root = Json::parse(q2s(result.responseBody));
    } catch (...) {
        return QString();
    }

    QString path = expr.startsWith(QLatin1Char('$')) ? expr.mid(1) : expr;
    if (path.startsWith(QLatin1Char('.'))) path = path.mid(1);
    if (path.isEmpty()) return result.responseBody;

    std::vector<Json> matches;
    walk(root, tokenize(path), 0, matches);
    return matches.empty() ? QString() : render(matches.front());
}

} // namespace rcm
