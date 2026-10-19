#include "carbon/ledger.hpp"
#include <chrono>

namespace carbon::ledger {

SqliteLedger::SqliteLedger(fs::path database_path) : db_path_(std::move(database_path)) {}

SqliteLedger::~SqliteLedger() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

CarbonResult<void> SqliteLedger::InitializeDatabase() {
    std::error_code ec;
    fs::create_directories(db_path_.parent_path(), ec);

    int rc = sqlite3_open_v2(db_path_.c_str(), &db_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    if (rc != SQLITE_OK) {
        std::string err = db_ ? sqlite3_errmsg(db_) : "Nao foi possivel abrir banco";
        return CarbonResult<void>::err(ErrorCode::ERR_SQLITE_FAIL, "Failed to open SQLite database: " + err);
    }

    // Configuração de Pragmas de alta performance
    const char* pragmas = 
        "PRAGMA journal_mode = WAL;"
        "PRAGMA synchronous = NORMAL;"
        "PRAGMA foreign_keys = ON;"
        "PRAGMA busy_timeout = 5000;";

    char* errmsg = nullptr;
    rc = sqlite3_exec(db_, pragmas, nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        std::string err = errmsg ? errmsg : "Desconhecido";
        sqlite3_free(errmsg);
        return CarbonResult<void>::err(ErrorCode::ERR_SQLITE_FAIL, "Failed to apply PRAGMAs: " + err);
    }

    // Criação de Tabelas DDL
    const char* ddl = 
        "CREATE TABLE IF NOT EXISTS build_nodes ("
        "  node_id TEXT PRIMARY KEY,"
        "  layer_level INTEGER NOT NULL,"
        "  description TEXT NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS execution_cache ("
        "  node_id TEXT NOT NULL,"
        "  input_fingerprint TEXT NOT NULL,"
        "  artifact_path TEXT NOT NULL,"
        "  artifact_size_bytes INTEGER NOT NULL,"
        "  artifact_sha256 TEXT NOT NULL,"
        "  execution_duration_ms INTEGER NOT NULL,"
        "  last_build_timestamp INTEGER NOT NULL,"
        "  is_valid BOOLEAN NOT NULL DEFAULT 1,"
        "  PRIMARY KEY (node_id, input_fingerprint)"
        ");"
        "CREATE TABLE IF NOT EXISTS build_runs ("
        "  run_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  start_timestamp INTEGER NOT NULL,"
        "  finish_timestamp INTEGER,"
        "  target_device TEXT NOT NULL,"
        "  cli_flags_json TEXT NOT NULL,"
        "  total_time_saved_ms INTEGER DEFAULT 0,"
        "  exit_code INTEGER DEFAULT -1"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_cache_lookup "
        "  ON execution_cache(node_id, input_fingerprint, is_valid);";

    rc = sqlite3_exec(db_, ddl, nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        std::string err = errmsg ? errmsg : "Desconhecido";
        sqlite3_free(errmsg);
        return CarbonResult<void>::err(ErrorCode::ERR_SQLITE_FAIL, "Failed to create SQLite tables: " + err);
    }

    return CarbonResult<void>::ok();
}

CarbonResult<std::optional<CacheRecord>> SqliteLedger::QueryCache(const std::string& node_id, 
                                                                 const std::string& input_fingerprint) {
    if (!db_) {
        return CarbonError(ErrorCode::ERR_SQLITE_FAIL, "Banco de dados nao inicializado");
    }

    const char* sql = "SELECT artifact_path, artifact_size_bytes, artifact_sha256, execution_duration_ms, last_build_timestamp "
                      "FROM execution_cache WHERE node_id = ? AND input_fingerprint = ? AND is_valid = 1 LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return CarbonError(ErrorCode::ERR_SQLITE_FAIL, sqlite3_errmsg(db_));
    }

    sqlite3_bind_text(stmt, 1, node_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, input_fingerprint.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        CacheRecord rec;
        rec.node_id = node_id;
        rec.input_fingerprint = input_fingerprint;
        rec.artifact_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        rec.artifact_size = static_cast<uint64_t>(sqlite3_column_int64(stmt, 1));
        rec.artifact_sha256 = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        rec.execution_duration_ms = static_cast<uint64_t>(sqlite3_column_int64(stmt, 3));
        rec.timestamp_epoch = sqlite3_column_int64(stmt, 4);

        sqlite3_finalize(stmt);
        return std::make_optional(rec);
    }

    sqlite3_finalize(stmt);
    return std::optional<CacheRecord>(std::nullopt);
}

CarbonResult<void> SqliteLedger::CommitExecution(const CacheRecord& record) {
    if (!db_) {
        return CarbonResult<void>::err(ErrorCode::ERR_SQLITE_FAIL, "Banco de dados nao inicializado");
    }

    const char* sql = "INSERT OR REPLACE INTO execution_cache "
                      "(node_id, input_fingerprint, artifact_path, artifact_size_bytes, artifact_sha256, execution_duration_ms, last_build_timestamp, is_valid) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, 1);";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return CarbonResult<void>::err(ErrorCode::ERR_SQLITE_FAIL, sqlite3_errmsg(db_));
    }

    sqlite3_bind_text(stmt, 1, record.node_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, record.input_fingerprint.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, record.artifact_path.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 4, static_cast<sqlite3_int64>(record.artifact_size));
    sqlite3_bind_text(stmt, 5, record.artifact_sha256.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 6, static_cast<sqlite3_int64>(record.execution_duration_ms));
    sqlite3_bind_int64(stmt, 7, record.timestamp_epoch > 0 ? record.timestamp_epoch : std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return CarbonResult<void>::err(ErrorCode::ERR_SQLITE_FAIL, sqlite3_errmsg(db_));
    }

    return CarbonResult<void>::ok();
}

CarbonResult<void> SqliteLedger::InvalidateNode(const std::string& node_id) {
    if (!db_) return CarbonResult<void>::err(ErrorCode::ERR_SQLITE_FAIL, "Banco de dados nao inicializado");

    const char* sql = "UPDATE execution_cache SET is_valid = 0 WHERE node_id = ?;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, node_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return CarbonResult<void>::ok();
}

CarbonResult<void> SqliteLedger::PurgeAll() {
    if (!db_) return CarbonResult<void>::err(ErrorCode::ERR_SQLITE_FAIL, "Banco de dados nao inicializado");

    char* errmsg = nullptr;
    int rc = sqlite3_exec(db_, "DELETE FROM execution_cache; VACUUM;", nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        std::string err = errmsg ? errmsg : "Desconhecido";
        sqlite3_free(errmsg);
        return CarbonResult<void>::err(ErrorCode::ERR_SQLITE_FAIL, "Failed to purge cache: " + err);
    }

    return CarbonResult<void>::ok();
}

CarbonResult<int64_t> SqliteLedger::BeginBuildRun(const std::string& target_device, const std::string& flags_json) {
    if (!db_) return CarbonError(ErrorCode::ERR_SQLITE_FAIL, "Banco de dados nao inicializado");

    const char* sql = "INSERT INTO build_runs (start_timestamp, target_device, cli_flags_json) VALUES (?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);

    int64_t now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    sqlite3_bind_int64(stmt, 1, now);
    sqlite3_bind_text(stmt, 2, target_device.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, flags_json.c_str(), -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return CarbonError(ErrorCode::ERR_SQLITE_FAIL, sqlite3_errmsg(db_));
    }

    return sqlite3_last_insert_rowid(db_);
}

CarbonResult<void> SqliteLedger::CompleteBuildRun(int64_t run_id, int exit_code, uint64_t time_saved_ms) {
    if (!db_) return CarbonResult<void>::err(ErrorCode::ERR_SQLITE_FAIL, "Banco de dados nao inicializado");

    const char* sql = "UPDATE build_runs SET finish_timestamp = ?, exit_code = ?, total_time_saved_ms = ? WHERE run_id = ?;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);

    int64_t now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    sqlite3_bind_int64(stmt, 1, now);
    sqlite3_bind_int(stmt, 2, exit_code);
    sqlite3_bind_int64(stmt, 3, static_cast<sqlite3_int64>(time_saved_ms));
    sqlite3_bind_int64(stmt, 4, run_id);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return CarbonResult<void>::ok();
}

} // namespace carbon::ledger
