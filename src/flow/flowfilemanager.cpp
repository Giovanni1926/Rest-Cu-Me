#include "flowfilemanager.h"

namespace rcm {

static QString jsonString(const Json& o, const QString& key) {
    if (!o.is_object()) return QString();
    auto it = o.find(q2s(key));
    if (it == o.end() || it->is_null()) return QString();
    if (it->is_string()) return s2q(it->get<std::string>());
    return QString();
}

static int jsonInt(const Json& o, const QString& key, int def) {
    if (!o.is_object()) return def;
    auto it = o.find(q2s(key));
    if (it == o.end() || !it->is_number_integer()) return def;
    return it->get<int>();
}

static Json jsonValue(const Json& o, const QString& key) {
    if (!o.is_object()) return Json();
    auto it = o.find(q2s(key));
    if (it == o.end()) return Json();
    return *it;
}

FlowNode FlowFileManager::parseNode(const Json& o) {
    QString id = jsonString(o, QStringLiteral("id"));
    int x = jsonInt(o, QStringLiteral("x"), 0);
    int y = jsonInt(o, QStringLiteral("y"), 0);
    QString title = jsonString(o, QStringLiteral("title"));
    QString kind = jsonString(o, QStringLiteral("kind"));
    Json call = jsonValue(o, QStringLiteral("call"));
    VarMap vars;
    Json varsObj = jsonValue(o, QStringLiteral("vars"));
    if (varsObj.is_object()) vars = fromJsonObject(varsObj);
    return FlowNode(id, x, y, title, kind, call, vars);
}

FlowEdge FlowFileManager::parseEdge(const Json& o) {
    QString id = jsonString(o, QStringLiteral("id"));
    QString from = jsonString(o, QStringLiteral("from"));
    QString to = jsonString(o, QStringLiteral("to"));
    std::optional<std::vector<FlowOutput>> outputs;
    Json out = jsonValue(o, QStringLiteral("outputs"));
    if (out.is_array()) {
        std::vector<FlowOutput> list;
        for (const auto& item : out) {
            list.emplace_back(jsonString(item, QStringLiteral("name")),
                              jsonString(item, QStringLiteral("expr")));
        }
        outputs = list;
    } else if (!out.is_null()) {
        std::vector<FlowOutput> list;
        outputs = list;
    }
    return FlowEdge(id, from, to, outputs);
}

FlowGroup FlowFileManager::parseGroup(const Json& o) {
    QString id = jsonString(o, QStringLiteral("id"));
    QString name = jsonString(o, QStringLiteral("name"));
    int x = jsonInt(o, QStringLiteral("x"), 0);
    int y = jsonInt(o, QStringLiteral("y"), 0);
    int w = jsonInt(o, QStringLiteral("w"), 0);
    int h = jsonInt(o, QStringLiteral("h"), 0);
    std::vector<QString> nodeIds;
    Json ids = jsonValue(o, QStringLiteral("nodeIds"));
    if (ids.is_array()) {
        for (const auto& i : ids) {
            if (i.is_string()) nodeIds.push_back(s2q(i.get<std::string>()));
        }
    }
    VarMap vars;
    Json varsObj = jsonValue(o, QStringLiteral("variables"));
    if (varsObj.is_object()) vars = fromJsonObject(varsObj);
    return FlowGroup(id, name, x, y, w, h, nodeIds, vars);
}

Json FlowFileManager::nodeToJson(const FlowNode& n) {
    Json o = Json::object();
    o["id"] = q2s(n.id);
    o["x"] = n.x;
    o["y"] = n.y;
    if (!n.title.isEmpty()) o["title"] = q2s(n.title);
    o["kind"] = q2s(n.kind);
    if (!n.call.is_null()) o["call"] = n.call;
    o["vars"] = toJsonObject(n.vars);
    return o;
}

Json FlowFileManager::edgeToJson(const FlowEdge& e) {
    Json o = Json::object();
    o["id"] = q2s(e.id);
    o["from"] = q2s(e.from);
    o["to"] = q2s(e.to);
    if (e.outputs) {
        Json arr = Json::array();
        for (const auto& out : *e.outputs) {
            Json io = Json::object();
            io["name"] = q2s(out.name);
            io["expr"] = q2s(out.expr);
            arr.push_back(io);
        }
        o["outputs"] = arr;
    }
    return o;
}

Json FlowFileManager::groupToJson(const FlowGroup& g) {
    Json o = Json::object();
    o["id"] = q2s(g.id);
    o["name"] = q2s(g.name);
    o["x"] = g.x;
    o["y"] = g.y;
    o["w"] = g.w;
    o["h"] = g.h;
    Json ids = Json::array();
    for (const auto& id : g.nodeIds) ids.push_back(q2s(id));
    o["nodeIds"] = ids;
    o["variables"] = toJsonObject(g.variables);
    return o;
}

QString FlowFileManager::toJson(const FlowGraph& graph) {
    Json root = Json::object();
    root["variables"] = graph.variables();
    Json nodes = Json::array();
    for (const auto& n : graph.nodes()) nodes.push_back(nodeToJson(n));
    Json edges = Json::array();
    for (const auto& e : graph.edges()) edges.push_back(edgeToJson(e));
    Json groups = Json::array();
    for (const auto& g : graph.groups()) groups.push_back(groupToJson(g));
    root["nodes"] = nodes;
    root["edges"] = edges;
    root["groups"] = groups;
    return s2q(root.dump(2));
}

FlowGraph FlowFileManager::load(const QString& text) {
    FlowGraph graph;
    try {
        Json parsed = Json::parse(q2s(text));
        if (parsed.is_null() || !parsed.is_object()) return graph;
        migrate(parsed);
        graph.variables() = jsonValue(parsed, QStringLiteral("variables")).is_object()
                                ? jsonValue(parsed, QStringLiteral("variables"))
                                : Json::object();
        Json nodesArr = jsonValue(parsed, QStringLiteral("nodes"));
        if (nodesArr.is_array()) {
            for (const auto& n : nodesArr) graph.nodes().push_back(parseNode(n));
        }
        Json edgesArr = jsonValue(parsed, QStringLiteral("edges"));
        if (edgesArr.is_array()) {
            for (const auto& e : edgesArr) graph.edges().push_back(parseEdge(e));
        }
        Json groupsArr = jsonValue(parsed, QStringLiteral("groups"));
        if (groupsArr.is_array()) {
            for (const auto& g : groupsArr) graph.groups().push_back(parseGroup(g));
        }
        graph.ensureCollections();
    } catch (...) {
        return FlowGraph();
    }
    return graph;
}

Json FlowFileManager::asArray(const Json& element) {
    if (element.is_array()) return element;
    return Json();
}

QString FlowFileManager::stringOf(const Json& object, const QString& field) {
    return jsonString(object, field);
}

bool FlowFileManager::contains(const Json& array, const QString& value) {
    if (!array.is_array()) return false;
    for (const auto& item : array) {
        if (item.is_string() && s2q(item.get<std::string>()) == value) return true;
    }
    return false;
}

Json FlowFileManager::mapping(const QString& name, const QString& expr) {
    Json o = Json::object();
    o["name"] = q2s(name);
    o["expr"] = q2s((expr.isEmpty()) ? QStringLiteral("$") : expr);
    return o;
}

void FlowFileManager::migrate(Json& root) {
    if (!root.is_object()) return;
    Json& nodes = root["nodes"];
    Json& edges = root["edges"];
    if (!nodes.is_array()) nodes = Json::array();
    if (!edges.is_array()) edges = Json::array();

    for (auto& node : nodes) {
        if (!node.is_object()) continue;
        auto it = node.find("call");
        if (it != node.end() && it->is_null()) node.erase(it);
    }

    Json nodeOutputs = Json::object();
    Json variableNode = Json::object();
    for (auto& node : nodes) {
        if (!node.is_object()) continue;
        QString id = jsonString(node, QStringLiteral("id"));
        if (id.isEmpty()) continue;
        auto outIt = node.find("outputs");
        if (outIt != node.end() && outIt->is_array()) {
            nodeOutputs[q2s(id)] = *outIt;
        } else {
            nodeOutputs[q2s(id)] = Json::array();
        }
        bool isVar = jsonString(node, QStringLiteral("kind")) == FlowNode::KIND_VARIABLES;
        variableNode[q2s(id)] = isVar;
    }

    for (auto& edge : edges) {
        if (!edge.is_object()) continue;
        auto outIt = edge.find("outputs");
        if (outIt != edge.end()) {
            edge.erase("vars");
            edge.erase("expr");
            edge.erase("var");
            continue;
        }

        QString from = jsonString(edge, QStringLiteral("from"));
        bool varSource = false;
        auto vnIt = variableNode.find(q2s(from));
        if (vnIt != variableNode.end()) varSource = vnIt->get<bool>();

        Json mappings = Json::array();

        bool hasExpr = edge.find("expr") != edge.end();
        bool hasVar = edge.find("var") != edge.end();
        if (hasExpr || hasVar) {
            QString var = jsonString(edge, QStringLiteral("var"));
            QString expr = jsonString(edge, QStringLiteral("expr"));
            if (!var.isEmpty()) {
                mappings.push_back(mapping(var, expr));
            }
        } else if (!varSource) {
            auto soIt = nodeOutputs.find(q2s(from));
            Json sourceOutputs = Json::array();
            if (soIt != nodeOutputs.end()) sourceOutputs = soIt.value();
            auto varsIt = edge.find("vars");
            bool all = (varsIt == edge.end() || varsIt->is_null());
            Json varsList = Json::array();
            if (!all && varsIt->is_array()) varsList = *varsIt;
            for (const auto& output : sourceOutputs) {
                QString name = jsonString(output, QStringLiteral("name"));
                if (name.isEmpty()) continue;
                if (all || contains(varsList, name)) {
                    mappings.push_back(mapping(name, jsonString(output, QStringLiteral("expr"))));
                }
            }
        }

        if (!varSource) {
            edge["outputs"] = mappings;
        } else {
            edge.erase("outputs");
        }
        edge.erase("vars");
        edge.erase("expr");
        edge.erase("var");
    }

    for (auto& node : nodes) {
        if (node.is_object()) node.erase("outputs");
    }
}

} // namespace rcm
