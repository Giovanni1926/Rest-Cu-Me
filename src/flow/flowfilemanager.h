#pragma once

#include <QString>
#include "flowgraph.h"

namespace rcm {

class FlowFileManager {
public:
    static FlowGraph load(const QString& text);
    static QString toJson(const FlowGraph& graph);

private:
    static void migrate(Json& root);
    static Json mapping(const QString& name, const QString& expr);
    static bool contains(const Json& array, const QString& value);
    static Json asArray(const Json& element);
    static QString stringOf(const Json& object, const QString& field);

    static FlowNode parseNode(const Json& o);
    static FlowEdge parseEdge(const Json& o);
    static FlowGroup parseGroup(const Json& o);
    static Json nodeToJson(const FlowNode& n);
    static Json edgeToJson(const FlowEdge& e);
    static Json groupToJson(const FlowGroup& g);
};

} // namespace rcm
