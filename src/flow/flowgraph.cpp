#include "flowgraph.h"

namespace rcm {

FlowNode* FlowGraph::nodeById(const QString& id) {
    for (auto& n : nodes_) {
        if (n.id == id) return &n;
    }
    return nullptr;
}

const FlowNode* FlowGraph::nodeById(const QString& id) const {
    for (const auto& n : nodes_) {
        if (n.id == id) return &n;
    }
    return nullptr;
}

FlowEdge* FlowGraph::edgeById(const QString& id) {
    for (auto& e : edges_) {
        if (e.id == id) return &e;
    }
    return nullptr;
}

FlowGroup* FlowGraph::groupById(const QString& id) {
    for (auto& g : groups_) {
        if (g.id == id) return &g;
    }
    return nullptr;
}

FlowGroup* FlowGraph::groupOf(const QString& nodeId) {
    for (auto& g : groups_) {
        for (const auto& id : g.nodeIds) {
            if (id == nodeId) return &g;
        }
    }
    return nullptr;
}

const FlowGroup* FlowGraph::groupOf(const QString& nodeId) const {
    for (const auto& g : groups_) {
        for (const auto& id : g.nodeIds) {
            if (id == nodeId) return &g;
        }
    }
    return nullptr;
}

void FlowGraph::replaceNode(const FlowNode& updated) {
    for (auto& n : nodes_) {
        if (n.id == updated.id) {
            n = updated;
            return;
        }
    }
}

void FlowGraph::moveNode(const QString& id, int x, int y) {
    for (auto& n : nodes_) {
        if (n.id == id) {
            n = n.moveTo(x, y);
            return;
        }
    }
}

void FlowGraph::removeNode(const QString& id) {
    nodes_.erase(std::remove_if(nodes_.begin(), nodes_.end(),
                                [&](const FlowNode& n) { return n.id == id; }),
                 nodes_.end());
    edges_.erase(std::remove_if(edges_.begin(), edges_.end(),
                                [&](const FlowEdge& e) {
                                    return e.from == id || e.to == id;
                                }),
                 edges_.end());
    for (auto& g : groups_) {
        auto ids = g.nodeIds;
        ids.erase(std::remove(ids.begin(), ids.end(), id), ids.end());
        g = g.withNodeIds(ids);
    }
}

void FlowGraph::removeEdge(const QString& id) {
    edges_.erase(std::remove_if(edges_.begin(), edges_.end(),
                                [&](const FlowEdge& e) { return e.id == id; }),
                 edges_.end());
}

void FlowGraph::addGroup(const FlowGroup& group) {
    groups_.push_back(group);
}

void FlowGraph::removeGroup(const QString& id) {
    groups_.erase(std::remove_if(groups_.begin(), groups_.end(),
                                 [&](const FlowGroup& g) { return g.id == id; }),
                  groups_.end());
}

void FlowGraph::moveGroup(const QString& id, int dx, int dy) {
    for (auto& g : groups_) {
        if (g.id != id) continue;
        g = g.moveTo(g.x + dx, g.y + dy);
        std::vector<QString> members = g.nodeIds;
        for (const auto& nodeId : members) {
            if (FlowNode* n = nodeById(nodeId)) {
                n->x += dx;
                n->y += dy;
            }
        }
        return;
    }
}

void FlowGraph::resizeGroup(const QString& id, int x, int y, int w, int h) {
    for (auto& g : groups_) {
        if (g.id == id) {
            g = g.resize(x, y, w, h);
            return;
        }
    }
}

void FlowGraph::renameGroup(const QString& id, const QString& name) {
    for (auto& g : groups_) {
        if (g.id == id) {
            g = g.withName(name);
            return;
        }
    }
}

void FlowGraph::updateGroupVariables(const QString& id, const VarMap& variables) {
    for (auto& g : groups_) {
        if (g.id == id) {
            g = g.withVariables(variables);
            return;
        }
    }
}

void FlowGraph::setNodeGroup(const QString& nodeId, const QString& groupId) {
    for (auto& g : groups_) {
        auto ids = g.nodeIds;
        const bool member =
            std::find(ids.begin(), ids.end(), nodeId) != ids.end();
        if (g.id == groupId) {
            if (!member) ids.push_back(nodeId);
        } else if (member) {
            ids.erase(std::remove(ids.begin(), ids.end(), nodeId), ids.end());
        }
        g = g.withNodeIds(ids);
    }
}

std::vector<FlowEdge> FlowGraph::incomingEdges(const QString& nodeId) const {
    std::vector<FlowEdge> result;
    for (const auto& e : edges_) {
        if (e.to == nodeId) result.push_back(e);
    }
    return result;
}

std::vector<FlowEdge> FlowGraph::outgoingEdges(const QString& nodeId) const {
    std::vector<FlowEdge> result;
    for (const auto& e : edges_) {
        if (e.from == nodeId) result.push_back(e);
    }
    return result;
}

} // namespace rcm
