#pragma once

#include <QString>
#include <vector>
#include <functional>
#include <map>
#include <set>
#include "flowgraph.h"
#include "../model/variabledecl.h"
#include "../http/restclient.h"
#include "../run/consoleline.h"

namespace rcm {

class RcmFlowRunService {
public:
    static RcmFlowRunService& instance();

    std::map<QString, CallResult> getResults() const { return results_; }

    int addListener(const std::function<void()>& listener);
    void removeListener(int id);

    void runAll(const FlowGraph& graph, const QString& title);
    void runFrom(const FlowGraph& graph, const QString& startNodeId, const QString& title);

    static std::vector<VariableDecl> buildRawContext(
        const FlowNode& node, const FlowGraph& graph,
        const std::map<QString, std::vector<FlowEdge>>& incoming,
        const std::map<QString, CallResult>& runResults,
        const std::map<QString, VarMap>& variableOutputs);
    static std::vector<VariableDecl> resolveContext(const std::vector<VariableDecl>& raw);
    static std::vector<QString> unresolvedVariables(const Json& call,
                                                    const std::vector<VariableDecl>& context);
    static QString firstUnresolvedVariable(const Json& call,
                                           const std::vector<VariableDecl>& context);

    struct FlowOutputEvent {
        bool isResult = false;
        ConsoleLine line;
        CallResult result;
        QString label;
    };

private:

    void run(const FlowGraph& graph, const QString& startNodeId, const QString& title);

    static QString resolveDeep(const QString& value, const std::vector<VariableDecl>& context,
                               std::set<QString>& seen);
    static VarMap resolveVariableNode(const FlowNode& node,
                                      const std::vector<VariableDecl>& context);
    static void scanUnresolved(const Json& element, const std::vector<VariableDecl>& context,
                               std::set<QString>& unresolved, std::set<QString>& visiting);
    static void scanText(const QString& text, const std::vector<VariableDecl>& context,
                         std::set<QString>& unresolved, std::set<QString>& visiting);
    static QString firstValue(const std::vector<VariableDecl>& context, const QString& name);
    static CallResult errorResult(const FlowNode& node, const QString& message);
    static QString labelOf(const FlowNode& node);
    static QString soapOperationOf(const FlowNode& node);
    static QString normalize(const Json& value);

    std::map<QString, CallResult> results_;
    std::vector<std::pair<int, std::function<void()>>> listeners_;
    int nextListenerId_ = 1;
};

} // namespace rcm
