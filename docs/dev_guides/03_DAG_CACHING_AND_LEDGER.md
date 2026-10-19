# 📊 DAG Caching, SQLite Ledger & Cryptographic State

**UN1CA Carbon Release:** IX (One UI 9.0 • Android 17 • SDK 37)  
**Organization:** ALRI Development ([ALRIGROUP](https://github.com/alrigroup))  
**Authors:** Alexsander "AlexsAR" ALRI (`@alexsanderalri`), ALRI Systems Engineering Team  
**Standard:** OpenSSL EVP / SQLite 3.39+ WAL  

---

## 1. Principles of Deterministic ROM Caching

In traditional ROM compilation, modifying a single configuration file (such as `rezoss_bootanimation.sh` or a wallpaper) often triggers a complete re-execution of all modules, spending 20 to 30 minutes recompiling every decompiled APK (`SecSettings`, `SystemUI`, `Camera`, `SmartSuggestions`, etc.).

**Carbon Engine** resolves this inefficiency by treating the entire build as a **Directed Acyclic Graph (DAG)** of atomic tasks, verified by a persistent SQLite ledger.

```
       [Firmware Verify] (SHA-256)
               │
               ▼
       [WorkDir Snapshot] (rsync)
               │
         ┌─────┴─────────────┐
         ▼                   ▼
   [Core Patches]     [Vendor Modules]
         │                   │
         └─────┬─────────────┘
               │
               ▼
   [APK/JAR Parallel Worker Pool]  <── (Ledger Cache: 24 APK SHA-256s)
               │
               ▼
   [Partitions Composer] (EROFS LZ4)
               │
               ▼
   [Direct ZIP Package & Sign]
```

---

## 2. The Phase-Skipping Behavior: "Can Phases 1 to 79 be Skipped?"

> **Question:** *"If a change is made at Step 80, are Steps 1 through 79 completely skipped?"*

### The Answer: **YES, with mathematical precision.**

When executing in incremental DAG mode (invoking `./carbon_engine/make_carbon_engine.sh` without the `--force` flag):

1. **Topological Traversal:** The engine processes tasks in topological order.
2. **Fingerprint Evaluation:** For each task $T_i$ from $i = 1$ to $79$, `InputHasher` computes the composite SHA-256 hash of its input files and parameters.
3. **Ledger Lookup:** The engine queries `execution_cache` in `ledger.db` for the key `(node_id, input_fingerprint)`.
4. **Instantaneous Bypass (`CACHE_HIT`):** Because nothing in Steps 1 through 79 has changed on disk, all 79 queries return `true`. Each step takes **< 0.05 milliseconds** to evaluate.
5. **Execution at Step 80:** When the engine reaches Step 80, the SHA-256 hash of the modified file differs from the record in `ledger.db`. The cache query returns `false` (`CACHE_MISS`).
6. **Cascade:** Step 80 and any downstream dependent steps (such as filesystem packaging) are executed and committed to the ledger with their new fingerprints.

```
┌─────────┬───────────────────────────────┬─────────────┬─────────────┐
│ STEP ID │ OPERATION                     │ DURATION    │ STATUS      │
├─────────┼───────────────────────────────┼─────────────┼─────────────┤
│ step_01 │ Verify firmware sha256        │ 0.001s      │ CACHE_HIT   │
│ step_02 │ Unpack kernel/dtbo            │ 0.001s      │ CACHE_HIT   │
│ ...     │ ...                           │ ...         │ ...         │
│ step_79 │ Apply floating_feature configs│ 0.001s      │ CACHE_HIT   │
│ step_80 │ Update bootanimation script   │ 0.012s      │ EXECUTED    │
│ step_81 │ Stage 7: EROFS system.img     │ 84.120s     │ EXECUTED    │
└─────────┴───────────────────────────────┴─────────────┴─────────────┘
```

---

## 3. SQLite Ledger Architecture (`SqliteLedger`)

The ledger is located at `${PROJECT_ROOT}/out/.carbon_cache/ledger.db`.

### 3.1. Pragma Configuration
To ensure maximum concurrent I/O performance on NVMe drives without locking conflicts:
* `PRAGMA journal_mode = WAL;` (Write-Ahead Logging allows simultaneous readers and writers)
* `PRAGMA synchronous = NORMAL;` (Ensures durability without excessive fsync flushes)
* `PRAGMA foreign_keys = ON;` (Guarantees referential integrity)
* `PRAGMA temp_store = MEMORY;` (Keeps temporary tables in RAM)

### 3.2. Database Schema
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

CREATE INDEX IF NOT EXISTS idx_cache_fingerprint ON execution_cache(input_fingerprint);
```

---

## 4. Cryptographic Input Fingerprinting (`InputHasher`)

`InputHasher` utilizes OpenSSL's high-speed **EVP SHA-256** engine (`EVP_sha256()`).

### 4.1. File Hashing
Reads files in 64 KB streaming buffers:
```cpp
CarbonResult<std::string> InputHasher::HashFile(const fs::path& path) {
    if (!fs::exists(path) || !fs::is_regular_file(path)) {
        return CarbonResult<std::string>::err(ErrorCode::ERR_STAT_SCAN_FAILED, 
            "File does not exist: " + path.string());
    }
    std::ifstream file(path, std::ios::binary);
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(mdctx, EVP_sha256(), nullptr);

    char buffer[65536];
    while (file.read(buffer, sizeof(buffer))) {
        EVP_DigestUpdate(mdctx, buffer, file.gcount());
    }
    if (file.gcount() > 0) {
        EVP_DigestUpdate(mdctx, buffer, file.gcount());
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len = 0;
    EVP_DigestFinal_ex(mdctx, hash, &hash_len);
    EVP_MD_CTX_free(mdctx);

    return CarbonResult<std::string>::ok(BytesToHex(hash, hash_len));
}
```

### 4.2. Directory & Smali Hashing
For directories (such as decompiled APK trees), `InputHasher::HashDirectory` performs a sorted recursive traversal:
1. Gathers all file paths relative to root.
2. Sorts paths lexicographically (`LC_ALL=C`).
3. For each file, hashes its relative path, file size, permissions, and byte contents.
4. Feeds all sub-hashes into a master SHA-256 context.

This guarantees that whitespace or metadata tampering is detected, while unchanged directories yield identical hashes regardless of filesystem modification timestamps.

---

## 5. Ledger CLI Commands

The Carbon Engine binary includes built-in commands to query and manipulate the cache directly from the terminal:

### 5.1. Inspect Cache Records
Displays the total number of cached tasks, recent build runs, and time saved:
```bash
./carbon_engine/build/carbon-engine inspect
```

### 5.2. Invalidate a Single Node
If you want to force re-execution of a specific node without rebuilding the entire ROM:
```bash
./carbon_engine/build/carbon-engine clean --clean-node=apk:SecSettings.apk
```

### 5.3. Purge All Cache (`Clean Rebuild`)
Wipes `ledger.db` completely:
```bash
./carbon_engine/build/carbon-engine clean
# Or run with force flag:
./carbon_engine/make_carbon_engine.sh --target=dm1q --force
```
