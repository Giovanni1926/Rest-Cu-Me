#include "flowplanner.h"
#include <map>
#include <queue>
#include <set>

namespace rcm {

std::vector<QString> FlowPlanner::reachable(const FlowGraph& graph, const QString& startId,
                                            const std::map<QString, FlowNode>& nodeMap) {
    std::set<QString> visited;
    std::deque<QString> queue;
    visited.insert(startId);
    queue.push_back(startId);
    while (!queue.empty()) {
        const QString id = queue.front();
        queue.pop_front();
        for (const auto& e : graph.outgoingEdges(id)) {
            if (nodeMap.count(e.to) && visited.insert(e.to).second) {
                queue.push_back(e.to);
            }
        }
    }
    return std::vector<QString>(visited.begin(), visited.end());
}

std::vector<QString> FlowPlanner::withUpstreamVariableNodes(
    const FlowGraph& graph, const std::vector<QString>& active,
    const std::map<QString, FlowNode>& nodeMap) {
    std::set<QString> extended(active.begin(), active.end());
    std::deque<QString> queue(active.begin(), active.end());
    while (!queue.empty()) {
        const QString id = queue.front();
        queue.pop_front();
        for (const auto& edge : graph.incomingEdges(id)) {
            auto it = nodeMap.find(edge.from);
            if (it == nodeMap.end()) continue;
            if (it->second.isVariableNode() && extended.insert(edge.from).second) {
                queue.push_back(edge.from);
            }
        }
    }
    return std::vector<QString>(extended.begin(), extended.end());
}

std::vector<FlowNode> FlowPlanner::plan(const FlowGraph& graph, const QString& startNodeId,
                                        std::vector<QString>& errors) {
    std::map<QString, FlowNode> nodeMap;
    for (const auto& n : graph.nodes()) {
        nodeMap[n.id] = n;
    }

    if (!startNodeId.isNull() && !nodeMap.count(startNodeId)) {
        errors.push_back(QStringLiteral("Nodo non trovato: ") + startNodeId);
        return {};
    }

    std::vector<QString> active;
    if (startNodeId.isNull()) {
        for (const auto& n : graph.nodes()) active.push_back(n.id);
    } else {
        active = reachable(graph, startNodeId, nodeMap);
        active = withUpstreamVariableNodes(graph, active, nodeMap);
    }

    std::set<QString> activeSet(active.begin(), active.end());

    std::map<QString, int> inDegree;
    std::map<QString, std::vector<QString>> dependents;
    for (const auto& e : graph.edges()) {
        if (!activeSet.count(e.from) || !activeSet.count(e.to)) continue;
        dependents[e.from].push_back(e.to);
        inDegree[e.to]++;
    }

    std::deque<QString> queue;
    for (const auto& id : active) {
        if (inDegree[id] == 0) queue.push_back(id);
    }

    std::vector<FlowNode> order;
    while (!queue.empty()) {
        const QString id = queue.front();
        queue.pop_front();
        order.push_back(nodeMap[id]);
        for (const auto& next : dependents[id]) {
            if (--inDegree[next] == 0) queue.push_back(next);
        }
    }

    if (static_cast<int>(order.size()) != static_cast<int>(active.size())) {
        errors.push_back(
            QStringLiteral("Ciclo rilevato nel flusso: impossibile determinare l'ordine di esecuzione"));
    }
    return order;
}

} // namespace rcm
