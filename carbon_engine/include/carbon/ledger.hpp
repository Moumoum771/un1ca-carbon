#pragma once
#include <string>
#include <optional>
#include <filesystem>
#include <sqlite3.h>
#include "carbon/common.hpp"

namespace carbon::ledger {

struct CacheRecord {
    std::string node_id;
    std::string input_fingerprint;
    std::string artifact_path;
    uint64_t    artifact_size{0};
    std::string artifact_sha256;
    uint64_t    execution_duration_ms{0};
    int64_t     timestamp_epoch{0};
};

class SqliteLedger {
public:
    explicit SqliteLedger(fs::path database_path);
    ~SqliteLedger();

    // Inicializa o banco de dados e cria tabelas se não existirem
    CarbonResult<void> InitializeDatabase();

    // Consulta se uma tarefa já possui resultado válido em cache
    CarbonResult<std::optional<CacheRecord>> QueryCache(const std::string& node_id, 
                                                        const std::string& input_fingerprint);

    // Registra o sucesso de uma tarefa recém-compilada
    CarbonResult<void> CommitExecution(const CacheRecord& record);

    // Invalida explicitamente entradas de cache
    CarbonResult<void> InvalidateNode(const std::string& node_id);
    CarbonResult<void> PurgeAll();

    // Inicia e encerra sessão de compilação
    CarbonResult<int64_t> BeginBuildRun(const std::string& target_device, const std::string& flags_json);
    CarbonResult<void> CompleteBuildRun(int64_t run_id, int exit_code, uint64_t time_saved_ms);

private:
    fs::path db_path_;
    ::sqlite3* db_{nullptr};
};

} // namespace carbon::ledger
