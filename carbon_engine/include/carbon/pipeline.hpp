#pragma once
#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include <chrono>
#include "carbon/common.hpp"
#include "carbon/ledger.hpp"
#include "carbon/dag.hpp"
#include "carbon/fingerprint.hpp"
#include "carbon/supervisor.hpp"
#include "carbon/key_manager.hpp"

namespace carbon::pipeline {

struct PipelineConfig {
    std::string target_device{"dm1q"};
    std::string bootanim_style{"carbon"};
    bool debloat_telemetry{true};
    bool force_rebuild{false};
    bool dry_run{false};
    uint32_t concurrency_jobs{0}; // 0 = auto-detect
    std::string stage_filter;     // vazio = all, ou "patches", "modules", "apks", "images", "package"
    std::optional<fs::path> custom_keys_dir;
};

struct StageMetrics {
    std::string stage_name;
    uint64_t elapsed_ms{0};
    bool skipped_cache_hit{false};
    std::string details;
};

class PipelineOrchestrator {
public:
    explicit PipelineOrchestrator(fs::path project_root, PipelineConfig config);
    ~PipelineOrchestrator();

    // Executa todo o pipeline do zero ou incrementalmente
    CarbonResult<void> RunPipeline();

    // Executa estágios individuais
    CarbonResult<void> Stage0_DiscoverEnvironment();
    CarbonResult<void> Stage1_SetupTree();
    CarbonResult<void> Stage2_VerifyFirmware();
    CarbonResult<void> Stage3_InitWorkDir();
    CarbonResult<void> Stage4_ApplyPatches();
    CarbonResult<void> Stage5_ApplyModules();
    CarbonResult<void> Stage6_CompileApksAndJars();
    CarbonResult<void> Stage7_ComposePartitions();
    CarbonResult<void> Stage8_AssemblePackage();

    const std::vector<StageMetrics>& GetMetrics() const { return metrics_; }
    fs::path GetFinalZipPath() const { return final_zip_path_; }
    std::string GetCompositeFingerprint() const { return composite_fingerprint_; }

private:
    fs::path project_root_;
    PipelineConfig config_;
    std::unique_ptr<ledger::SqliteLedger> ledger_;
    std::unique_ptr<supervisor::ProcessSupervisor> supervisor_;
    std::unique_ptr<security::KeyManager> key_manager_;
    security::KeyBundle active_keys_;

    fs::path out_dir_;
    fs::path work_dir_;
    fs::path apktool_dir_;
    fs::path fw_dir_;
    fs::path tools_dir_;
    fs::path final_zip_path_;
    std::string pda_version_{"S911BXXUAZZI8"};
    std::string composite_fingerprint_;

    std::vector<StageMetrics> metrics_;
    uint64_t total_time_saved_ms_{0};

    void record_metric(const std::string& name, uint64_t ms, bool cached, const std::string& info = "");
};

} // namespace carbon::pipeline
