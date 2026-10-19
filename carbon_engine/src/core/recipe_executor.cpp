#include "carbon/recipe.hpp"
#include "carbon/fingerprint.hpp"
#include "carbon/supervisor.hpp"
#include "nlohmann/json.hpp"
#include <fstream>
#include <iostream>
#include <regex>
#include <iomanip>

using json = nlohmann::json;

namespace carbon::recipe {

RecipeExecutor::RecipeExecutor(fs::path project_root, fs::path recipe_file, ledger::SqliteLedger& ledger)
    : project_root_(std::move(project_root)), recipe_file_(std::move(recipe_file)), ledger_(ledger) {}

RecipeExecutor::~RecipeExecutor() = default;

CarbonResult<RecipeManifest> RecipeExecutor::LoadRecipe() {
    if (!fs::exists(recipe_file_)) {
        return CarbonError(ErrorCode::ERR_HASH_IO_ERROR, "Recipe manifest file not found: " + recipe_file_.string());
    }

    std::ifstream f(recipe_file_);
    if (!f.is_open()) {
        return CarbonError(ErrorCode::ERR_HASH_IO_ERROR, "Cannot open recipe manifest for reading: " + recipe_file_.string());
    }

    try {
        json j = json::parse(f);
        RecipeManifest manifest;
        manifest.project_name = j.value("project", "UN1CA Carbon IX");
        manifest.target_device = j.value("target", "dm1q");
        manifest.engine_version = j.value("engine_version", "1.0.0-PROD");

        if (j.contains("pipeline") && j["pipeline"].is_array()) {
            uint32_t auto_step_idx = 1;
            for (const auto& item : j["pipeline"]) {
                RecipeStep step;
                step.step_number = auto_step_idx++; // Calculado automaticamente pela posicao no array
                step.id = item.value("id", "");
                step.category = item.value("category", "General");
                step.description = item.value("description", "");
                step.credit = item.value("credit", "");
                step.action = item.value("action", "");

                step.source = item.value("source", "");
                step.destination = item.value("destination", "");
                step.file = item.value("file", "");
                step.key = item.value("key", "");
                step.value = item.value("value", "");
                step.search_pattern = item.value("search", "");
                step.replace_pattern = item.value("replace", "");
                step.script_path = item.value("script", "");
                step.condition = item.value("condition", "");
                step.apk = item.value("apk", "");
                step.patch = item.value("patch", "");

                if (item.contains("args") && item["args"].is_array()) {
                    for (const auto& arg : item["args"]) {
                        step.script_args.push_back(arg.get<std::string>());
                    }
                }

                if (item.contains("metadata") && item["metadata"].is_object()) {
                    StepMetadata meta;
                    meta.uid = item["metadata"].value("uid", 0);
                    meta.gid = item["metadata"].value("gid", 0);
                    std::string m_str = item["metadata"].value("mode", "0644");
                    meta.mode = static_cast<uint32_t>(std::stoul(m_str, nullptr, 8));
                    meta.selabel = item["metadata"].value("selabel", "u:object_r:system_file:s0");
                    step.metadata = meta;
                }

                manifest.steps.push_back(step);
            }
        }

        return manifest;
    } catch (const std::exception& e) {
        return CarbonError(ErrorCode::ERR_INTERNAL_ERROR, "Failed to parse recipe JSON: " + std::string(e.what()));
    }
}

bool RecipeExecutor::EvaluateCondition(const std::string& condition, 
                                       const std::unordered_map<std::string, std::string>& env_vars) {
    if (condition.empty()) return true;

    // Parser simples de igualdade: env.KEY == 'VALUE'
    std::regex re("env\\.([A-Za-z0-9_]+)\\s*==\\s*'([^']*)'");
    std::smatch m;
    if (std::regex_search(condition, m, re)) {
        std::string var_name = m[1].str();
        std::string expected = m[2].str();
        auto it = env_vars.find(var_name);
        std::string actual = (it != env_vars.end()) ? it->second : "";
        return actual == expected;
    }

    return true;
}

std::string RecipeExecutor::CalculateStepFingerprint(const RecipeStep& step, 
                                                   const std::unordered_map<std::string, std::string>& env_vars) {
    std::string seed = step.action + ":" + step.id;

    if (!step.source.empty()) {
        fs::path p = project_root_ / step.source;
        if (fs::exists(p)) {
            fingerprint::InputHasher h(p);
            auto res = h.ComputeDeepContentHash();
            if (res) seed += ":" + res.value();
        } else if (fs::exists(p.string() + ".00")) {
            auto res0 = fingerprint::InputHasher::ComputeFileSha256(p.string() + ".00");
            if (res0) seed += ":split:" + res0.value();
        }
    }

    if (!step.key.empty()) seed += ":" + step.key + "=" + step.value;
    if (!step.search_pattern.empty()) seed += ":" + step.search_pattern + "->" + step.replace_pattern;
    if (!step.script_path.empty()) {
        fs::path sp = project_root_ / step.script_path;
        if (fs::exists(sp)) {
            auto s_res = fingerprint::InputHasher::ComputeFileSha256(sp);
            if (s_res) seed += ":" + s_res.value();
        }
        for (const auto& arg : step.script_args) seed += ":" + arg;
    }

    if (!step.patch.empty()) {
        fs::path pp = project_root_ / step.patch;
        if (fs::exists(pp)) {
            auto p_res = fingerprint::InputHasher::ComputeFileSha256(pp);
            if (p_res) seed += ":" + p_res.value();
        }
        seed += ":apk:" + step.apk;
    }

    for (const auto& [k, v] : env_vars) {
        seed += ";env:" + k + "=" + v;
    }

    fs::path tmp_seed_file = fs::temp_directory_path() / ("seed_" + step.id + ".tmp");
    std::ofstream sf(tmp_seed_file);
    sf << seed;
    sf.close();

    auto f_res = fingerprint::InputHasher::ComputeFileSha256(tmp_seed_file);
    fs::remove(tmp_seed_file);

    return f_res.value_or("0000000000000000000000000000000000000000000000000000000000000000");
}

CarbonResult<void> RecipeExecutor::ActionFsCopy(const RecipeStep& step) {
    fs::path src = project_root_ / step.source;
    fs::path dst = project_root_ / step.destination;

    std::error_code ec;
    fs::create_directories(dst.parent_path(), ec);

    if (!fs::exists(src)) {
        fs::path part0 = src;
        part0 += ".00";
        if (fs::exists(part0)) {
            // Reconstroi arquivos divididos em partes (.00, .01...)
            std::ofstream out(dst, std::ios::binary | std::ios::trunc);
            int part_idx = 0;
            while (true) {
                char ext[16];
                snprintf(ext, sizeof(ext), ".%02d", part_idx);
                fs::path curr_part = src;
                curr_part += ext;
                if (!fs::exists(curr_part)) break;
                std::ifstream part_file(curr_part, std::ios::binary);
                out << part_file.rdbuf();
                part_idx++;
            }
            out.close();
        } else {
            return CarbonResult<void>::err(ErrorCode::ERR_HASH_IO_ERROR, "Source does not exist: " + src.string());
        }
    } else {
        if (fs::is_directory(src)) {
            fs::copy(src, dst, fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
        } else {
            fs::copy_file(src, dst, fs::copy_options::overwrite_existing, ec);
        }

        if (ec) {
            return CarbonResult<void>::err(ErrorCode::ERR_HASH_IO_ERROR, "Failed to copy: " + ec.message());
        }
    }

    // Grava metadados no fs_config se definidos
    if (step.metadata) {
        fs::path work_configs = project_root_ / "out" / "target" / "dm1q" / "work_dir" / "configs";
        if (fs::exists(work_configs)) {
            std::string rel_dst = fs::relative(dst, project_root_ / "out" / "target" / "dm1q" / "work_dir").string();
            std::string part = rel_dst.substr(0, rel_dst.find('/'));
            fs::path fsc_file = work_configs / ("fs_config-" + part);
            std::ofstream fsc(fsc_file, std::ios::app);
            if (fsc.is_open()) {
                fsc << rel_dst << " " << step.metadata->uid << " " << step.metadata->gid << " 0" 
                    << std::oct << step.metadata->mode << std::dec << " capabilities=0x0\n";
            }
        }
    }

    return CarbonResult<void>::ok();
}

CarbonResult<void> RecipeExecutor::ActionFsDelete(const RecipeStep& step) {
    fs::path target = project_root_ / step.file;
    std::error_code ec;
    fs::remove_all(target, ec);
    return CarbonResult<void>::ok();
}

CarbonResult<void> RecipeExecutor::ActionPropSet(const RecipeStep& step) {
    fs::path target = project_root_ / step.file;
    if (!fs::exists(target)) {
        std::error_code ec;
        fs::create_directories(target.parent_path(), ec);
        std::ofstream nf(target);
        nf.close();
    }

    std::ifstream in(target);
    std::string line;
    std::vector<std::string> lines;
    bool replaced = false;

    while (std::getline(in, line)) {
        if (line.rfind(step.key + "=", 0) == 0) {
            lines.push_back(step.key + "=" + step.value);
            replaced = true;
        } else {
            lines.push_back(line);
        }
    }
    in.close();

    if (!replaced) {
        lines.push_back(step.key + "=" + step.value);
    }

    std::ofstream out(target, std::ios::trunc);
    for (const auto& l : lines) out << l << "\n";
    out.close();

    return CarbonResult<void>::ok();
}

CarbonResult<void> RecipeExecutor::ActionFloatingFeatureSet(const RecipeStep& step) {
    fs::path target = project_root_ / step.file;
    if (!fs::exists(target)) return CarbonResult<void>::ok();

    std::ifstream in(target);
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    std::string tag = "<" + step.key + ">";
    std::string close_tag = "</" + step.key + ">";
    std::string new_entry = tag + step.value + close_tag;

    size_t pos = content.find(tag);
    if (pos != std::string::npos) {
        size_t end_pos = content.find(close_tag, pos);
        if (end_pos != std::string::npos) {
            content.replace(pos, (end_pos + close_tag.length()) - pos, new_entry);
        }
    } else {
        size_t close_set = content.rfind("</SecFloatingFeatureSet>");
        if (close_set != std::string::npos) {
            content.insert(close_set, "    " + new_entry + "\n");
        }
    }

    std::ofstream out(target, std::ios::trunc);
    out << content;
    out.close();

    return CarbonResult<void>::ok();
}

CarbonResult<void> RecipeExecutor::ActionSmaliPatch(const RecipeStep& step) {
    fs::path target = project_root_ / step.file;
    if (!fs::exists(target)) return CarbonResult<void>::ok();

    std::ifstream in(target);
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    size_t pos = content.find(step.search_pattern);
    if (pos != std::string::npos) {
        content.replace(pos, step.search_pattern.length(), step.replace_pattern);
        std::ofstream out(target, std::ios::trunc);
        out << content;
        out.close();
    }

    return CarbonResult<void>::ok();
}

CarbonResult<void> RecipeExecutor::ActionScriptRun(const RecipeStep& step) {
    supervisor::ProcessSupervisor supervisor;
    supervisor::CommandSpec spec;
    spec.executable = step.script_path;
    spec.args = step.script_args;
    spec.work_dir = project_root_;

    auto res = supervisor.ExecuteSynchronous(spec);
    if (!res || res.value().exit_code != 0) {
        return CarbonResult<void>::err(ErrorCode::ERR_SUBPROCESS_FAIL, 
            "Script falhou: " + (res ? res.value().std_err : ""));
    }
    return CarbonResult<void>::ok();
}

CarbonResult<void> RecipeExecutor::ActionApkPatch(const RecipeStep& step) {
    fs::path patch_path = project_root_ / step.patch;
    fs::path apk_decoded_dir = project_root_ / "out" / "target" / "dm1q" / "apktool" / step.apk;

    if (!fs::exists(patch_path)) {
        return CarbonResult<void>::err(ErrorCode::ERR_HASH_IO_ERROR, "Patch not found: " + patch_path.string());
    }

    if (!fs::exists(apk_decoded_dir)) {
        // If decoded folder does not exist yet in current phase, validation ok
        return CarbonResult<void>::ok();
    }

    supervisor::ProcessSupervisor supervisor;
    supervisor::CommandSpec spec;
    spec.executable = "patch";
    spec.args = { "-p1", "-N", "-r", "-", "-d", apk_decoded_dir.string(), "-i", patch_path.string() };
    spec.work_dir = project_root_;

    auto res = supervisor.ExecuteSynchronous(spec);
    // patch -p1 -N returns 0 on success or 1 if already applied
    if (res && res.value().exit_code != 0 && res.value().exit_code != 1) {
        return CarbonResult<void>::err(ErrorCode::ERR_SUBPROCESS_FAIL, 
            "Failed to apply patch to APK " + step.apk + ": " + res.value().std_err);
    }
    return CarbonResult<void>::ok();
}

CarbonResult<bool> RecipeExecutor::ExecuteStep(const RecipeStep& step, 
                                             const std::unordered_map<std::string, std::string>& env_vars) {
    if (!EvaluateCondition(step.condition, env_vars)) {
        return false; // Ignorado por condição
    }

    std::string fp = CalculateStepFingerprint(step, env_vars);
    auto q_res = ledger_.QueryCache("step:" + step.id, fp);

    // Validação de Cache Hit
    if (q_res && q_res.value().has_value()) {
        // Se a ação tiver um destino no filesystem, checa se ainda existe fisicamente
        if (!step.destination.empty()) {
            if (fs::exists(project_root_ / step.destination)) {
                return true; // Cache HIT confirmado!
            }
        } else {
            return true;
        }
    }

    // Executa ação nativa correspondente
    CarbonResult<void> action_res = CarbonResult<void>::ok();

    if (step.action == "fs.copy") action_res = ActionFsCopy(step);
    else if (step.action == "fs.delete") action_res = ActionFsDelete(step);
    else if (step.action == "prop.set") action_res = ActionPropSet(step);
    else if (step.action == "floating_feature.set") action_res = ActionFloatingFeatureSet(step);
    else if (step.action == "smali.patch") action_res = ActionSmaliPatch(step);
    else if (step.action == "script.run") action_res = ActionScriptRun(step);
    else if (step.action == "apk.patch") action_res = ActionApkPatch(step);

    if (!action_res) return action_res.error();

    // Grava no Ledger
    ledger::CacheRecord rec;
    rec.node_id = "step:" + step.id;
    rec.input_fingerprint = fp;
    rec.artifact_path = step.destination.empty() ? step.file : step.destination;
    rec.artifact_size = 1024;
    rec.artifact_sha256 = fp;
    rec.execution_duration_ms = 10;
    ledger_.CommitExecution(rec);

    return true;
}

std::string RecipeExecutor::ResolveMacros(const std::string& input, const std::unordered_map<std::string, std::string>& macro_map) {
    if (input.empty()) return input;
    std::string result = input;
    for (const auto& [macro, replacement] : macro_map) {
        size_t pos = 0;
        while ((pos = result.find(macro, pos)) != std::string::npos) {
            result.replace(pos, macro.length(), replacement);
            pos += replacement.length();
        }
    }
    return result;
}

void RecipeExecutor::ResolveStepMacros(RecipeStep& step, const std::unordered_map<std::string, std::string>& macro_map) {
    step.source = ResolveMacros(step.source, macro_map);
    step.destination = ResolveMacros(step.destination, macro_map);
    step.file = ResolveMacros(step.file, macro_map);
    step.value = ResolveMacros(step.value, macro_map);
    step.script_path = ResolveMacros(step.script_path, macro_map);
    step.apk = ResolveMacros(step.apk, macro_map);
    step.patch = ResolveMacros(step.patch, macro_map);
    for (auto& arg : step.script_args) {
        arg = ResolveMacros(arg, macro_map);
    }
}

CarbonResult<void> RecipeExecutor::ExecuteAll(const std::unordered_map<std::string, std::string>& env_vars) {
    auto m_res = LoadRecipe();
    if (!m_res) return m_res.error();

    auto manifest = m_res.value();
    size_t hits = 0;
    size_t executed = 0;
    size_t total_steps = manifest.steps.size();

    std::string target = env_vars.count("TARGET") ? env_vars.at("TARGET") : manifest.target_device;
    if (target.empty()) target = "dm1q";

    std::string fw_path = (target == "dm3q") ? "SM-S918B_EUX" : "SM-S911B_OXM";
    if (!fs::exists(project_root_ / "out" / "fw" / fw_path)) {
        std::error_code ec;
        for (const auto& entry : fs::directory_iterator(project_root_ / "out" / "fw", ec)) {
            if (entry.is_directory()) {
                fw_path = entry.path().filename().string();
                break;
            }
        }
    }

    std::unordered_map<std::string, std::string> macro_map = {
        {"${TARGET}", target},
        {"${WORK_DIR}", "out/target/" + target + "/work_dir"},
        {"${APKTOOL_DIR}", "out/target/" + target + "/apktool"},
        {"${FW_DIR}", "out/fw/" + fw_path},
        {"${OUT_DIR}", "out"}
    };

    json report_json;
    report_json["project"] = manifest.project_name;
    report_json["target"] = target;
    report_json["total_steps"] = total_steps;
    report_json["steps"] = json::array();

    std::cout << "\033[1;36m============================================================\n"
              << "       CARBON ENGINE — EXECUÇÃO DE RECEITA DECLARATIVA      \n"
              << "       Projeto: " << manifest.project_name << " (" << target << ")\n"
              << "       Total de Passos Dinâmicos: " << total_steps << "\n"
              << "============================================================\033[0m\n\n";

    for (auto step : manifest.steps) {
        ResolveStepMacros(step, macro_map);
        auto res = ExecuteStep(step, env_vars);
        if (!res) {
            std::cerr << "\n\033[1;31m[FALHA NO PASSO " << step.step_number << "/" << total_steps 
                      << " (" << step.id << ")] " << res.error().to_string() << "\033[0m\n";
            return res.error();
        }

        std::string tag = res.value() ? "\033[1;32mCACHE_HIT\033[0m" : "\033[1;34mEXECUTED\033[0m ";
        if (res.value()) hits++; else executed++;

        std::cout << "• Passo [" << std::setw(3) << step.step_number << "/" << total_steps << "] [" << tag << "] " 
                  << std::left << std::setw(30) << step.id.substr(0, 30) 
                  << " (" << step.category << ")\n";

        if (!step.credit.empty()) {
            std::cout << "  \033[0;37m↳ Credit: " << step.credit << "\033[0m\n";
        }

        json step_entry;
        step_entry["sequence"] = step.step_number;
        step_entry["id"] = step.id;
        step_entry["category"] = step.category;
        step_entry["action"] = step.action;
        step_entry["status"] = res.value() ? "CACHE_HIT" : "EXECUTED";
        report_json["steps"].push_back(step_entry);
    }

    report_json["summary"] = {
        {"cached_steps", hits},
        {"executed_steps", executed},
        {"cache_hit_ratio", total_steps > 0 ? (static_cast<double>(hits) / total_steps) * 100.0 : 0.0}
    };

    fs::path manifest_path = project_root_ / "out" / "recipe_manifest.json";
    std::ofstream mf(manifest_path);
    if (mf.is_open()) {
        mf << report_json.dump(2);
        mf.close();
    }

    std::cout << "\n\033[1;32m✔ Recipe executed: " << hits << " steps preserved via Cache, " 
              << executed << " steps updated!\033[0m\n"
              << "✔ Manifest saved at: " << manifest_path << "\n\n";

    return CarbonResult<void>::ok();
}

} // namespace carbon::recipe
