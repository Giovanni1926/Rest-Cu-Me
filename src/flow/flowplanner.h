#pragma once

#include <QString>
#include <vector>
#include <map>
#include "flowgraph.h"

namespace rcm {

class FlowPlanner {
public:
    static std::vector<FlowNode> plan(const FlowGraph& graph, const QString& startNodeId,
                                      std::vector<QString>& errors);

private:
    static std::vector<QString> reachable(const FlowGraph& graph, const QString& startId,
                                          const std::map<QString, FlowNode>& nodeMap);
    static std::vector<QString> withUpstreamVariableNodes(
        const FlowGraph& graph, const std::vector<QString>& active,
        const std::map<QString, FlowNode>& nodeMap);
};

} // namespace rcm
