# ⚡ Carbon Engine Architecture & Core Subsystems

**UN1CA Carbon Release:** IX (One UI 9.0 • Android 17 • SDK 37)  
**Organization:** ALRI Development ([ALRIGROUP](https://github.com/alrigroup))  
**Authors:** Alexsander "AlexsAR" ALRI (`@alexsanderalri`), ALRI Systems Engineering Team  
**Standard:** C++20 / POSIX.1-2008  

---

## 1. Executive Summary & Design Rationale

Historical Android ROM kitchens and OEM modification frameworks (such as lineage-based tools, stock UN1CA bash scripts, or Android Kitchen) rely on monolithic, imperative Bash/Zsh scripts. While functional for small patches, these script suites suffer from fatal architectural flaws when scaling to multi-gigabyte modern firmwares:

1. **Lack of Dependency Analysis (DAG):** Traditional bash loops execute sequentially or spawn uncoordinated background subshells (`&`) with primitive `wait $(jobs -p)` barriers, risking race conditions and silent error swallowing.
2. **Missing Cryptographic State Tracking:** Scripts rely on directory presence (`[ -d folder ]`) or arbitrary timestamp files (`.completed`), causing either unnecessary full recompilations or corrupted partial builds when intermediate files are modified.
3. **Hardcoded Device Profiles:** Target variables (`dm1q`, `SM-S911B`, `/out/fw/...`) are tightly coupled inside nested scripts, requiring repetitive code duplication for sibling hardware variants (`dm2q` / S23+, `dm3q` / S23 Ultra).
4. **Fragile Process Supervision:** Child processes (such as `apktool`, `aapt2`, `brotli`, `fsck.erofs`) communicate exit statuses poorly through subshell layers, resulting in corrupted images progressing to packaging despite fatal errors.

**Carbon Engine** is a ground-up, native C++20 declarative orchestrator engineered by ALRI Development to replace brittle shell pipelines with a high-performance, deterministic build system featuring SQLite-backed DAG caching, cryptographic fingerprinting, and strict process isolation.

```
┌────────────────────────────────────────────────────────────────────────┐
│                          CARBON ENGINE CLI                             │
│       carbon-engine build | recipe | keys | inspect | clean | verify   │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
           ┌────────────────────────┴────────────────────────┐
           ▼                                                 ▼
┌───────────────────────────────┐         ┌───────────────────────────────┐
│     PipelineOrchestrator      │         │        RecipeExecutor         │
│   (9 Macroscopic Stages)      │         │  (160 Declarative Steps DAG)  │
└──────────────┬────────────────┘         └───────────────┬───────────────┘
               │                                          │
               └────────────────────┬─────────────────────┘
                                    │
    ┌───────────────────────────────┼──────────────────────────────┐
    ▼                               ▼                              ▼
┌─────────────────────────┐  ┌─────────────────────────┐  ┌─────────────────────────┐
│      SqliteLedger       │  │       InputHasher       │  │    ProcessSupervisor    │
│  SQLite3 (WAL Mode)     │  │  OpenSSL EVP SHA-256    │  │  POSIX fork/exec/waitpid │
│  .carbon_cache/ledger.db│  │  AST & Blob Fingerprint │  │  Worker Pool & Pipes     │
└─────────────────────────┘  └─────────────────────────┘  └─────────────────────────┘
```

---

## 2. Directory Hierarchy & Code Structure

The Carbon Engine codebase is organized inside the `carbon_engine/` subtree of the ROM repository:

```
carbon_engine/
├── CMakeLists.txt                 # Modern CMake build recipe (C++20, SQLite3, OpenSSL)
├── Carbonfile.json                # Master 160-step declarative recipe specification
├── make_carbon_engine.sh          # Auto-compiling shell bootstrap entrypoint
├── specs.md                       # Formal API specification and contracts
├── include/
│   └── carbon/
│       ├── common.hpp             # CarbonResult<T>, ErrorCode, and core types
│       ├── ledger.hpp             # SQLite3 state ledger and cache records
│       ├── hasher.hpp             # OpenSSL SHA-256 cascading hasher
│       ├── dag.hpp                # Directed Acyclic Graph topology engine
│       ├── supervisor.hpp         # POSIX process isolation and worker pool
│       ├── security.hpp           # Cryptographic platform key manager
│       ├── recipe.hpp             # JSON action deserializer and executor
│       └── pipeline.hpp           # High-level 9-stage build pipeline
├── src/
│   ├── main.cpp                   # CLI router and argument parser
│   ├── ledger/sqlite_ledger.cpp   # Database schema, queries, and transactions
│   ├── fingerprint/input_hasher.cpp # Recursive filesystem and string hashing
│   ├── dag/dependency_graph.cpp   # Topological sort (Kahn's algorithm)
│   ├── supervisor/process_supervisor.cpp # Fork/execvp, pipe capture, worker pool
│   ├── security/key_manager.cpp   # AVB and APK certificate validation
│   └── core/
│       ├── recipe_executor.cpp    # Declarative action execution engine
│       └── pipeline.cpp           # Macroscopic stage sequencing
└── tests/
    └── test_engine.cpp            # Unit test suite verifying hashing, DB, and DAG
```

---

## 3. Core Engine Subsystems

### 3.1. Process Supervisor & Worker Pool (`ProcessSupervisor`)
Spawning unmonitored subshells in bash causes memory exhaustion and masked error codes. `ProcessSupervisor` wraps low-level POSIX process primitives (`fork`, `execvp`, `pipe2`, `waitpid`):

* **Standard Output/Error Interception:** Captures `stdout` and `stderr` streams non-blockingly via UNIX pipes without deadlocks.
* **Bounded Concurrency (Worker Pool):** For resource-intensive workloads like `apktool` (which spawns JVM instances), the supervisor enforces a concurrency limit (default: 4 concurrent jobs) to prevent out-of-memory (OOM) conditions while utilizing multi-core parallelism.
* **Deterministic Exit Code Propagation:** Every child PID is tracked in an internal table. If any job terminates with a non-zero exit code, the supervisor captures the exact failing command, logs its standard error, and aborts downstream stages immediately.

### 3.2. Cryptographic Ledger & State Cache (`SqliteLedger`)
The ledger is stored at `${PROJECT_ROOT}/out/.carbon_cache/ledger.db`. It runs in SQLite **WAL (Write-Ahead Logging)** mode with `NORMAL` synchronous pragma for high concurrent write throughput:

```sql
CREATE TABLE IF NOT EXISTS execution_cache (
    node_id TEXT PRIMARY KEY,
    input_fingerprint TEXT NOT NULL,
    output_fingerprint TEXT,
    execution_duration_ms INTEGER NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS build_runs (
    run_id INTEGER PRIMARY KEY AUTOINCREMENT,
    target_device TEXT NOT NULL,
    flags_json TEXT NOT NULL,
    exit_code INTEGER,
    time_saved_ms INTEGER,
    started_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    completed_at TIMESTAMP
);
```

Before executing any action:
1. `InputHasher` calculates the SHA-256 hash of all input dependencies (files, smali patches, config variables).
2. `SqliteLedger::QueryCache(node_id, input_hash)` is queried.
3. If an identical fingerprint is found, the node is tagged as **`CACHE_HIT`** and skipped in under 1 millisecond.
4. If changed or unrecorded, the action is executed, its runtime is clocked with `std::chrono::steady_clock`, and committed via `SqliteLedger::CommitExecution`.

### 3.3. Macro Resolver System
To ensure cross-device portability across Samsung flagships (`dm1q` / S23, `dm2q` / S23+, `dm3q` / S23 Ultra), `RecipeExecutor` and `PipelineOrchestrator` resolve dynamic path tokens before filesystem or shell execution:

| Macro Token | Resolved Dynamic Value |
| :--- | :--- |
| `${TARGET}` | Target codename (e.g., `dm1q`, `dm2q`, `dm3q`) |
| `${WORK_DIR}` | Out-of-tree staging directory (`out/target/<device>/work_dir`) |
| `${APKTOOL_DIR}` | Decompiled APK staging tree (`out/target/<device>/apktool`) |
| `${FW_DIR}` | Extracted base firmware repository (`out/fw/<model>_<csc>`) |
| `${OUT_DIR}` | Final package artifact directory (`out/`) |
| `${SRC_DIR}` | Root directory of the source codebase |

---

## 4. Macroscopic Pipeline Stages (0 to 8)

The pipeline orchestrates the build through nine deterministic stages:

```
[STAGE 0] Environment Discovery
    ├── Validates host binaries: java (17+), aapt2, 7z, brotli, rsync, fsck.erofs, magiskboot
    └── Discovers target device profile and crypto security keys
[STAGE 1] Symbiotic Tree Setup
    └── Re-establishes modular symlinks in `unica/` and `target/` without polluting git
[STAGE 2] Base Firmware Verification
    └── Verifies SHA-256 integrity of Samsung stock extracted firmware partitions
[STAGE 3] WorkDir Initialization
    └── Executes rsync snapshot from stock firmware; cleans decompiled APK staging tree
[STAGE 4] Core & ROM Patches Application
    └── Applies low-level architecture diffs, PROCA patches, and signature spoofing
[STAGE 5] Modules Orchestration
    └── Executes vendor modules (Rezoss, NPL, Carbon branding, debloaters, kernel engine)
[STAGE 6] Incremental APK/JAR Compiler (Worker Pool)
    └── Cleans .orig/.rej; decompiles, patches, and rebuilds APKs/JARs with strict PID checks
[STAGE 7] Partitions Composer (EROFS + Brotli)
    └── Generates sparse EROFS filesystem images, signs with AVB 2.0, compresses via Brotli
[STAGE 8] Final Package Assembly & Delivery
    └── Packages recovery.img, boot.img, updater-script, produces signed final flashable ZIP
```

---

## 5. Clean Rebuild (`--force`) vs Incremental DAG Caching

* **Clean Rebuild (`--force` or `--clean`):**  
  Invoking `./carbon_engine/make_carbon_engine.sh --target=dm1q --force` executes `SqliteLedger::PurgeAll()`. It forces `create_work_dir.sh` to repopulate the staging directory with pristine Samsung blobs, ensuring any previous failed experiments, corrupt smali diffs, or duplicate AAPT2 definitions are purged.
* **Incremental DAG Caching (Default):**  
  Invoking `./carbon_engine/make_carbon_engine.sh --target=dm1q` activates full caching. Stages 0 through 5 skip verified nodes, Stage 6 skips unmodified APKs/JARs, and only dirty partitions are recomposed in Stage 7, drastically reducing iteration cycle time.
