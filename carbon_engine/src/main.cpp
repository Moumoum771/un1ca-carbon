#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <iomanip>
#include <fstream>
#include "carbon/fingerprint.hpp"
#include "carbon/ledger.hpp"
#include "carbon/dag.hpp"
#include "carbon/supervisor.hpp"
#include "carbon/key_manager.hpp"
#include "carbon/pipeline.hpp"
#include "carbon/recipe.hpp"
#include "nlohmann/json.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;
using namespace carbon;

static void print_banner() {
    std::cout << "\033[1;36m"
              << "============================================================\n"
              << "          CARBON ENGINE — NEXT-GEN BUILD ORCHESTRATOR        \n"
              << "     Input-Addressed DAG Pipeline & Cryptographic Ledger     \n"
              << "   Organization: ALRI Development (ALRIGROUP) • AlexsAR      \n"
              << "============================================================\033[0m\n\n";
}

static void print_help() {
    std::cout << "USAGE:\n"
              << "  carbon-engine [SUBCOMMAND] [OPTIONS]\n\n"
              << "SUBCOMMANDS:\n"
              << "  build       Execute the incremental build pipeline (default)\n"
              << "  recipe      Execute declarative Carbonfile.json step by step\n"
              << "  keys        Manage and inspect cryptographic keys (PKCS#8)\n"
              << "  inspect     Display cache fingerprint history and metrics\n"
              << "  clean       Purge cache database and transient staging data\n"
              << "  verify      Verify database and filesystem integrity\n\n"
              << "OPTIONS FOR 'build' AND 'recipe':\n"
              << "  -t, --target <device>        Target device codename (default: dm1q)\n"
              << "  --no-cache                   Force rebuild ignoring ledger cache\n"
              << "  --clean-node <id>            Invalidate cache for specific component\n"
              << "  --bootanim <carbon|rezoss>   Set default boot animation (default: carbon)\n\n";
}

static fs::path find_project_root() {
    fs::path curr = fs::current_path();
    while (true) {
        if (fs::exists(curr / "make_carbon.sh") || fs::exists(curr / ".git")) {
            return curr;
        }
        if (!curr.has_parent_path() || curr == curr.parent_path()) break;
        curr = curr.parent_path();
    }
    return fs::current_path();
}

int main(int argc, char* argv[]) {
    print_banner();

    std::string subcommand = "build";
    std::string target_device = "dm1q";
    bool force_no_cache = false;
    std::string clean_node_id;
    std::string bootanim_style = "carbon";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            print_help();
            return 0;
        } else if (arg == "build" || arg == "recipe" || arg == "keys" || arg == "inspect" || arg == "clean" || arg == "verify") {
            subcommand = arg;
        } else if (arg == "-t" || arg == "--target") {
            if (i + 1 < argc) target_device = argv[++i];
        } else if (arg.rfind("--target=", 0) == 0) {
            target_device = arg.substr(9);
        } else if (arg == "--no-cache" || arg == "--force" || arg == "-f" || arg == "--clean") {
            force_no_cache = true;
        } else if (arg == "--clean-node") {
            if (i + 1 < argc) clean_node_id = argv[++i];
        } else if (arg == "--bootanim") {
            if (i + 1 < argc) bootanim_style = argv[++i];
        } else if (arg.rfind("--bootanim=", 0) == 0) {
            bootanim_style = arg.substr(11);
        }
    }

    fs::path project_root = find_project_root();
    fs::path cache_dir = project_root / "out" / ".carbon_cache";
    fs::path ledger_path = cache_dir / "ledger.db";

    ledger::SqliteLedger ledger(ledger_path);
    auto init_res = ledger.InitializeDatabase();
    if (!init_res) {
        std::cerr << "\033[1;31m[ERRO] " << init_res.error().to_string() << "\033[0m\n";
        return 1;
    }

    security::KeyManager key_mgr(project_root);

    // =========================================================================
    // SUBCOMANDO: RECIPE
    // =========================================================================
    if (subcommand == "recipe") {
        fs::path recipe_path = project_root / "carbon_engine" / "Carbonfile.json";
        recipe::RecipeExecutor executor(project_root, recipe_path, ledger);
        std::unordered_map<std::string, std::string> env_vars = {
            { "TARGET", target_device },
            { "BOOTANIMATION_STYLE", bootanim_style },
            { "DEBLOAT_TELEMETRY", "true" }
        };

        auto res = executor.ExecuteAll(env_vars);
        if (!res) {
            std::cerr << "\033[1;31m[ERRO] " << res.error().to_string() << "\033[0m\n";
            return 1;
        }
        return 0;
    }

    // =========================================================================
    // SUBCOMMAND: KEYS
    // =========================================================================
    if (subcommand == "keys") {
        std::cout << "\033[1;33m[SECURITY] Inspecting Cryptographic Keys...\033[0m\n";
        auto key_res = key_mgr.DiscoverActiveKeys();
        if (!key_res) {
            std::cout << "\033[1;31m[ERROR] No keys found: " << key_res.error().to_string() << "\033[0m\n";
            return 1;
        }

        auto bundle = key_res.value();
        std::cout << "• Active Profile:   " 
                  << (bundle.profile == security::KeyProfile::OFFICIAL_CARBON_KEYS ? "🟢 OFFICIAL_CARBON_KEYS" :
                      bundle.profile == security::KeyProfile::DEVELOPER_RELEASE_KEYS ? "🔵 DEVELOPER_RELEASE_KEYS" :
                      "🟡 AOSP_TEST_KEYS_FALLBACK") << "\n";
        std::cout << "• Platform PKCS#8:  " << bundle.platform_pk8 << "\n";
        std::cout << "• Platform X.509:   " << bundle.platform_pem << "\n";
        std::cout << "• DER Signature:    " << bundle.platform_der_hex.substr(0, 32) << "... (total: " 
                  << bundle.platform_der_hex.size() / 2 << " bytes)\n\n";

        if (bundle.profile == security::KeyProfile::AOSP_TEST_KEYS_FALLBACK) {
            std::cout << "\033[1;33m[NOTICE] This build uses public AOSP test keys (test-keys).\n"
                      << "         For official production, place private keys in security/keys/.\033[0m\n";
        }
        return 0;
    }

    // =========================================================================
    // SUBCOMMAND: INSPECT
    // =========================================================================
    if (subcommand == "inspect") {
        std::cout << "\033[1;33m[CACHE] Inspecting Artifact Cache Ledger...\033[0m\n";
        std::cout << "• Database: " << ledger_path << "\n\n";

        fs::path manifest_path = project_root / "out" / "build_manifest.json";
        if (fs::exists(manifest_path)) {
            try {
                std::ifstream mf(manifest_path);
                json j = json::parse(mf);
                std::cout << "============================================================\n"
                          << "       LATEST BUILD FORENSIC MANIFEST (CARBON ENGINE)       \n"
                          << "============================================================\n"
                          << "• Build Fingerprint:   \033[1;36m" << j.value("composite_build_fingerprint", "N/A") << "\033[0m\n"
                          << "• Target Device:       " << j.value("target_device", "N/A") << "\n"
                          << "• PDA Version:         " << j.value("pda_version", "N/A") << "\n"
                          << "• Timestamp (ISO):     " << j.value("timestamp_iso", "N/A") << "\n"
                          << "• Key Profile:         " << j.value("key_profile", "N/A") << "\n";
                if (j.contains("summary")) {
                    auto s = j["summary"];
                    std::cout << "• Final Package:       " << s.value("final_zip_path", "N/A") << "\n"
                              << "• ZIP SHA-256:         \033[1;33m" << s.value("final_zip_sha256", "N/A") << "\033[0m\n"
                              << "• Package Size:        " << s.value("final_zip_size_bytes", 0ULL) / (1024 * 1024) << " MB\n";
                }
                std::cout << "============================================================\n\n";
            } catch (...) {}
        }

        fs::path artifacts_dir = cache_dir / "artifacts";
        if (fs::exists(artifacts_dir)) {
            size_t count = 0;
            uint64_t total_size = 0;
            std::error_code ec;
            for (const auto& entry : fs::directory_iterator(artifacts_dir, ec)) {
                if (entry.is_regular_file()) {
                    count++;
                    total_size += fs::file_size(entry.path(), ec);
                    std::cout << "  📦 " << std::left << std::setw(60) << entry.path().filename().string()
                              << " (" << fs::file_size(entry.path(), ec) / (1024 * 1024) << " MB)\n";
                }
            }
            std::cout << "\n• Total in Cache: " << count << " artifacts (" 
                      << total_size / (1024 * 1024) << " MB)\n";
        } else {
            std::cout << "• No cached physical artifacts currently stored.\n";
        }
        return 0;
    }

    // =========================================================================
    // SUBCOMMAND: CLEAN
    // =========================================================================
    if (subcommand == "clean") {
        std::cout << "\033[1;33m[CACHE] Purging Ledger database...\033[0m\n";
        ledger.PurgeAll();
        std::cout << "\033[1;32m[SUCCESS] Cache completely cleaned!\033[0m\n";
        return 0;
    }

    // =========================================================================
    // SUBCOMMAND: BUILD
    // =========================================================================
    if (force_no_cache) {
        ledger.PurgeAll();
    }

    pipeline::PipelineConfig p_cfg;
    p_cfg.target_device = target_device;
    p_cfg.bootanim_style = bootanim_style;
    p_cfg.force_rebuild = force_no_cache;
    p_cfg.debloat_telemetry = true;

    pipeline::PipelineOrchestrator orchestrator(project_root, p_cfg);
    auto run_res = orchestrator.RunPipeline();
    if (!run_res) {
        std::cerr << "\n\033[1;31m[BUILD FAILED] " << run_res.error().to_string() << "\033[0m\n\n";
        return 1;
    }

    std::cout << "\033[1;32m[BUILD SUCCESS] Pipeline executed and verified successfully by Carbon Engine!\033[0m\n\n";
    return 0;
}
