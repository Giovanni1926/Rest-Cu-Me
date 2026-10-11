#pragma once

#include <QString>
#include <vector>
#include <optional>
#include "../util/str.h"

namespace rcm {

struct FlowOutput {
    QString name;
    QString expr;

    FlowOutput() = default;
    FlowOutput(const QString& name, const QString& expr) : name(name), expr(expr) {}
};

struct FlowNode {
    static inline const QString KIND_REST = QStringLiteral("rest");
    static inline const QString KIND_SOAP = QStringLiteral("soap");
    static inline const QString KIND_VARIABLES = QStringLiteral("variables");

    QString id;
    int x = 0;
    int y = 0;
    QString title;
    QString kind;
    Json call;
    VarMap vars;

    FlowNode() = default;
    FlowNode(const QString& id, int x, int y, const QString& title, const QString& kind,
             const Json& call, const VarMap& vars)
        : id(id), x(x), y(y), title(title),
          kind((kind.isNull() || kind.trimmed().isEmpty()) ? KIND_REST : kind),
          call(call), vars(vars) {}

    static FlowNode rest(const QString& id, int x, int y, const QString& title, const Json& call) {
        return FlowNode(id, x, y, title, KIND_REST, call, {});
    }

    static FlowNode variables(const QString& id, int x, int y, const QString& title,
                              const VarMap& vars) {
        return FlowNode(id, x, y, title, KIND_VARIABLES, Json(), vars);
    }

    bool isVariableNode() const { return kind == KIND_VARIABLES; }
    bool isSoapNode() const { return kind == KIND_SOAP; }
    bool isRestNode() const { return !isVariableNode() && !isSoapNode(); }

    FlowNode moveTo(int nx, int ny) const {
        return FlowNode(id, nx, ny, title, kind, call, vars);
    }
};

struct FlowEdge {
    QString id;
    QString from;
    QString to;
    std::optional<std::vector<FlowOutput>> outputs;

    FlowEdge() = default;
    FlowEdge(const QString& id, const QString& from, const QString& to,
             const std::optional<std::vector<FlowOutput>>& outputs)
        : id(id), from(from), to(to), outputs(outputs) {}

    bool passAll() const { return !outputs.has_value(); }
};

struct FlowGroup {
    QString id;
    QString name;
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    std::vector<QString> nodeIds;
    VarMap variables;

    FlowGroup() = default;
    FlowGroup(const QString& id, const QString& name, int x, int y, int w, int h,
              const std::vector<QString>& nodeIds, const VarMap& variables)
        : id(id), name(name), x(x), y(y), w(w), h(h), nodeIds(nodeIds), variables(variables) {}

    FlowGroup withNodeIds(const std::vector<QString>& ids) const {
        return FlowGroup(id, name, x, y, w, h, ids, variables);
    }
    FlowGroup withVariables(const VarMap& v) const {
        return FlowGroup(id, name, x, y, w, h, nodeIds, v);
    }
    FlowGroup withName(const QString& n) const {
        return FlowGroup(id, n, x, y, w, h, nodeIds, variables);
    }
    FlowGroup moveTo(int nx, int ny) const {
        return FlowGroup(id, name, nx, ny, w, h, nodeIds, variables);
    }
    FlowGroup resize(int nx, int ny, int nw, int nh) const {
        return FlowGroup(id, name, nx, ny, nw, nh, nodeIds, variables);
    }
};

class FlowGraph {
public:
    FlowGraph() = default;
    void ensureCollections() {}

    Json& variables() { return variables_; }
    const Json& variables() const { return variables_; }
    std::vector<FlowNode>& nodes() { return nodes_; }
    const std::vector<FlowNode>& nodes() const { return nodes_; }
    std::vector<FlowEdge>& edges() { return edges_; }
    const std::vector<FlowEdge>& edges() const { return edges_; }
    std::vector<FlowGroup>& groups() { return groups_; }
    const std::vector<FlowGroup>& groups() const { return groups_; }

    FlowNode* nodeById(const QString& id);
    const FlowNode* nodeById(const QString& id) const;
    FlowEdge* edgeById(const QString& id);
    FlowGroup* groupById(const QString& id);
    FlowGroup* groupOf(const QString& nodeId);
    const FlowGroup* groupOf(const QString& nodeId) const;

    void replaceNode(const FlowNode& updated);
    void moveNode(const QString& id, int x, int y);
    void removeNode(const QString& id);
    void removeEdge(const QString& id);
    void addGroup(const FlowGroup& group);
    void removeGroup(const QString& id);
    void moveGroup(const QString& id, int dx, int dy);
    void resizeGroup(const QString& id, int x, int y, int w, int h);
    void renameGroup(const QString& id, const QString& name);
    void updateGroupVariables(const QString& id, const VarMap& variables);
    void setNodeGroup(const QString& nodeId, const QString& groupId);

    std::vector<FlowEdge> incomingEdges(const QString& nodeId) const;
    std::vector<FlowEdge> outgoingEdges(const QString& nodeId) const;

private:
    Json variables_ = Json::object();
    std::vector<FlowNode> nodes_;
    std::vector<FlowEdge> edges_;
    std::vector<FlowGroup> groups_;
};

} // namespace rcm
