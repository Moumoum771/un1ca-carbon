#include "carbon/dag.hpp"
#include <queue>
#include <unordered_set>

namespace carbon::dag {

DependencyGraph::DependencyGraph() = default;
DependencyGraph::~DependencyGraph() = default;

CarbonResult<void> DependencyGraph::RegisterNode(std::shared_ptr<TaskNode> node) {
    if (!node || node->node_id.empty()) {
        return CarbonResult<void>::err(ErrorCode::ERR_INVALID_ARGUMENT, "No invalido ou com node_id vazio");
    }
    nodes_[node->node_id] = std::move(node);
    return CarbonResult<void>::ok();
}

CarbonResult<void> DependencyGraph::AddDependency(const std::string& consumer_id, 
                                                 const std::string& provider_id) {
    auto c_it = nodes_.find(consumer_id);
    auto p_it = nodes_.find(provider_id);

    if (c_it == nodes_.end()) {
        return CarbonResult<void>::err(ErrorCode::ERR_NODE_NOT_FOUND, "Consumer node not found: " + consumer_id);
    }
    if (p_it == nodes_.end()) {
        return CarbonResult<void>::err(ErrorCode::ERR_NODE_NOT_FOUND, "Provider node not found: " + provider_id);
    }

    c_it->second->upstream_dependencies.push_back(provider_id);
    p_it->second->downstream_dependents.push_back(consumer_id);

    return CarbonResult<void>::ok();
}

CarbonResult<std::vector<std::shared_ptr<TaskNode>>> DependencyGraph::ResolveTopologicalOrder() {
    std::unordered_map<std::string, size_t> in_degree;
    for (const auto& [id, node] : nodes_) {
        in_degree[id] = node->upstream_dependencies.size();
    }

    std::queue<std::string> ready_queue;
    for (const auto& [id, deg] : in_degree) {
        if (deg == 0) {
            ready_queue.push(id);
        }
    }

    std::vector<std::shared_ptr<TaskNode>> ordered;
    ordered.reserve(nodes_.size());

    while (!ready_queue.empty()) {
        std::string curr = ready_queue.front();
        ready_queue.pop();

        auto node = nodes_[curr];
        ordered.push_back(node);

        for (const auto& dep_id : node->downstream_dependents) {
            if (--in_degree[dep_id] == 0) {
                ready_queue.push(dep_id);
            }
        }
    }

    if (ordered.size() != nodes_.size()) {
        return CarbonError(ErrorCode::ERR_CYCLE_DETECTED, "Dependencia circular detectada no grafo de build");
    }

    return ordered;
}

void DependencyGraph::InvalidateDownstreamCascade(const std::string& changed_node_id) {
    std::unordered_set<std::string> visited;
    std::queue<std::string> q;
    q.push(changed_node_id);

    while (!q.empty()) {
        std::string curr = q.front();
        q.pop();

        if (visited.count(curr)) continue;
        visited.insert(curr);

        auto it = nodes_.find(curr);
        if (it != nodes_.end()) {
            it->second->state.store(NodeState::ENQUEUED_FOR_BUILD);
            for (const auto& dep : it->second->downstream_dependents) {
                q.push(dep);
            }
        }
    }
}

std::shared_ptr<TaskNode> DependencyGraph::GetNode(const std::string& node_id) const {
    auto it = nodes_.find(node_id);
    return it != nodes_.end() ? it->second : nullptr;
}

const std::unordered_map<std::string, std::shared_ptr<TaskNode>>& DependencyGraph::GetAllNodes() const {
    return nodes_;
}

} // namespace carbon::dag
