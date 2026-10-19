#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <atomic>
#include "carbon/common.hpp"

namespace carbon::dag {

enum class NodeState : uint8_t {
    PENDING = 0,
    EVALUATING_CACHE,
    SKIPPED_CACHE_HIT,
    ENQUEUED_FOR_BUILD,
    RUNNING,
    COMPLETED_SUCCESS,
    FAILED_ABORT
};

enum class LayerLevel : uint8_t {
    LAYER_0_BASE_FIRMWARE = 0,
    LAYER_1_CORE_FRAMEWORK = 1,
    LAYER_2_SYSTEM_APPS    = 2,
    LAYER_3_MEDIA_BRANDING = 3,
    LAYER_4_FILESYSTEM_IMG = 4,
    LAYER_5_FLASHABLE_ZIP  = 5
};

struct TaskNode {
    std::string node_id;
    LayerLevel layer{LayerLevel::LAYER_2_SYSTEM_APPS};
    std::string display_name;

    std::vector<std::string> upstream_dependencies;
    std::vector<std::string> downstream_dependents;

    std::vector<fs::path> input_paths;
    fs::path output_artifact;
    std::unordered_map<std::string, std::string> environment_flags;

    std::atomic<NodeState> state{NodeState::PENDING};
    std::string calculated_fingerprint;
    uint64_t duration_ms{0};
    int exit_code{0};
    std::string error_message;
};

class DependencyGraph {
public:
    DependencyGraph();
    ~DependencyGraph();

    // Adiciona um nó ao grafo
    CarbonResult<void> RegisterNode(std::shared_ptr<TaskNode> node);

    // Cria aresta: consumer depende de provider
    CarbonResult<void> AddDependency(const std::string& consumer_id, 
                                     const std::string& provider_id);

    // Retorna a ordem topológica de execução (Algoritmo de Kahn)
    CarbonResult<std::vector<std::shared_ptr<TaskNode>>> ResolveTopologicalOrder();

    // Invalida recursivamente todos os nós dependentes
    void InvalidateDownstreamCascade(const std::string& changed_node_id);

    // Busca um nó por ID
    std::shared_ptr<TaskNode> GetNode(const std::string& node_id) const;

    // Retorna todos os nós cadastrados
    const std::unordered_map<std::string, std::shared_ptr<TaskNode>>& GetAllNodes() const;

private:
    std::unordered_map<std::string, std::shared_ptr<TaskNode>> nodes_;
};

} // namespace carbon::dag
