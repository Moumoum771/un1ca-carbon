#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <optional>
#include "carbon/common.hpp"
#include "carbon/ledger.hpp"

namespace carbon::recipe {

struct StepMetadata {
    uint32_t uid{0};
    uint32_t gid{0};
    uint32_t mode{0644};
    std::string selabel{"u:object_r:system_file:s0"};
};

struct RecipeStep {
    uint32_t step_number{0};
    std::string id;
    std::string category;
    std::string description;
    std::string credit;
    std::string action;

    // Campos opcionais dependendo da ação
    std::string source;
    std::string destination;
    std::string file;
    std::string key;
    std::string value;
    std::string search_pattern;
    std::string replace_pattern;
    std::string script_path;
    std::vector<std::string> script_args;
    std::string condition;
    std::string apk;
    std::string patch;

    std::optional<StepMetadata> metadata;
};

struct RecipeManifest {
    std::string project_name{"UN1CA Carbon IX"};
    std::string target_device{"dm1q"};
    std::string engine_version{"1.0.0-PROD"};
    std::vector<RecipeStep> steps;
};

class RecipeExecutor {
public:
    explicit RecipeExecutor(fs::path project_root, fs::path recipe_file, ledger::SqliteLedger& ledger);
    ~RecipeExecutor();

    // Carrega e parseia o Carbonfile.json
    CarbonResult<RecipeManifest> LoadRecipe();

    // Executa o pipeline passo a passo com verificação de cache e hash
    CarbonResult<void> ExecuteAll(const std::unordered_map<std::string, std::string>& env_vars);

    // Executa um passo único
    CarbonResult<bool> ExecuteStep(const RecipeStep& step, const std::unordered_map<std::string, std::string>& env_vars);

private:
    fs::path project_root_;
    fs::path recipe_file_;
    ledger::SqliteLedger& ledger_;

    // Executores de Ações Nativas em C++
    CarbonResult<void> ActionFsCopy(const RecipeStep& step);
    CarbonResult<void> ActionFsDelete(const RecipeStep& step);
    CarbonResult<void> ActionFsSetMetadata(const RecipeStep& step);
    CarbonResult<void> ActionPropSet(const RecipeStep& step);
    CarbonResult<void> ActionFloatingFeatureSet(const RecipeStep& step);
    CarbonResult<void> ActionSmaliPatch(const RecipeStep& step);
    CarbonResult<void> ActionScriptRun(const RecipeStep& step);
    CarbonResult<void> ActionApkPatch(const RecipeStep& step);

    std::string ResolveMacros(const std::string& input, const std::unordered_map<std::string, std::string>& macro_map);
    void ResolveStepMacros(RecipeStep& step, const std::unordered_map<std::string, std::string>& macro_map);

    bool EvaluateCondition(const std::string& condition, const std::unordered_map<std::string, std::string>& env_vars);
    std::string CalculateStepFingerprint(const RecipeStep& step, const std::unordered_map<std::string, std::string>& env_vars);
};

} // namespace carbon::recipe
