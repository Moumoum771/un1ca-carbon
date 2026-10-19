# [CARBON ENGINE] — ARCHITECTURE & EXECUTION ENGINEERING PLAN
**Project:** Carbon Engine (Next-Generation Modular ROM Build & Pipeline Orchestrator)  
**Organization:** ALRI Development (ALRIGROUP)  
**Systems Engineering:** AlexsAR (@alexsanderalri) & Platform Team  
**Document Status:** Master Architecture Plan (Precursor to `specs.md`)  
**Date:** October 2026  

---

## 1. OVERVIEW AND MISSION

**Carbon Engine** is the compilation, orchestration, and large-scale incremental management engine for Custom ROMs, designed for the **UN1CA Carbon** ecosystem and open to the international Android community.

### The Problem It Solves
Currently, ported ROM build pipelines (One UI, MIUI, ColorOS, Pixel) operate in an imperative, blind fashion:
- Any single change (such as a string in an APK or a script line) forces full deconstruction, patching, rebuilding, and compression of the entire operating system.
- Builds that should take seconds take **20 to 30 minutes**, redundantly consuming 100% CPU, disk I/O, and RAM.
- There is zero cryptographic traceability of which artifacts actually changed between revisions.

### The Mission of Carbon Engine
Transform ROM compilation into an **Input-Addressed Directed Acyclic Graph (DAG)** compute tree, reducing incremental builds from **25 minutes to under 60 seconds**, with absolute determinism, professional cryptographic key management, and complete portability for community maintainers.

---

## 2. ARCHITECTURAL INVARIANTS & CORE PRINCIPLES

1. **Input-Driven Caching (Fingerprints originate from sources, not outputs):**  
   The hash of every artifact is calculated from its decompiled source code, modification scripts, environment variables, and compiler toolchain versions. Volatile output metadata (such as internal APK timestamps) never invalidates the cache.
2. **Determinism and Atomicity:**  
   Corrupted files or builds interrupted by `Ctrl+C` never pollute future cache state. Every artifact is generated in an isolated staging area (`.tmp`) and atomically promoted.
3. **Decoupled Security Management:**  
   Native support for maintainer private keys (`release-keys`) with secure fallback to public AOSP test keys (`test-keys`), preventing any credential leaks in version control.
4. **Strict Dependency Graph Propagation:**  
   If a base component changes (e.g., resource IDs in `framework-res`), all downstream dependent nodes are automatically invalidated in cascade.
5. **High-Performance Native Implementation:**  
   Designed natively in **C++20** with OpenSSL and SQLite, avoiding slow scripting interpreter overhead.

---

## 3. FEATURE SPECIFICATION & SUBSYSTEMS

### FEATURE 1: Composite Input Fingerprinting
* **Mechanism:**  
  For each component (APK, JAR, Module, Partition, or Kernel Image), Carbon Engine computes a **Composite SHA-256 Fingerprint**:
  $$\text{Fingerprint} = \text{SHA256}(\text{Source/Smali Tree} + \text{Patches Hash} + \text{customize.sh Hash} + \text{CLI Flags} + \text{Toolchain Hash})$$
* **Cache Operation:**
  - If the computed fingerprint matches the previous ledger entry, the step is marked as `CACHE_HIT (SKIPPED)` and the compiled artifact is linked/copied in **0.05s**.
  - If any byte changes, a `CACHE_MISS` is triggered, building only that specific node.

---

### FEATURE 2: Two-Tier Fast Metadata Stat-Cache
* **Mechanism:**  
  To avoid calculating SHA-256 over tens of thousands of `.smali` files on every run:
  1. **Tier 1 (Fast Stat-Cache):** The engine reads `mtime` (modification time) and `size` via direct low-level kernel syscalls (`lstat`). If metadata matches the prior run, the directory is confirmed unmodified in under **10 milliseconds**.
  2. **Tier 2 (Deep Content Hash):** Only files with modified `mtime` undergo cryptographic SHA-256 hashing to update the composite fingerprint.

---

### FEATURE 3: Layered Directed Acyclic Graph (DAG Pipeline)
* **Mechanism:**  
  The engine decomposes ROM building into strict precedence layers:
  - **Layer 0 — Base Firmware:** Original Samsung firmware extraction (immutable in cache).
  - **Layer 1 — Framework & Core APIs:** `framework.jar`, `services.jar`, `framework-res.apk`.
  - **Layer 2 — System Application Layer:** `SecSettings`, `SamsungCamera`, `DressRoom`, `SystemUI`, `KernelSU-Next`.
  - **Layer 3 — Media & Identity Layer:** Boot animations, system audio, wallpapers (`wallpaper-res.apk`).
  - **Layer 4 — OS Partition Generation:** `system.img`, `vendor.img`, `product.img`, etc., generated via `mkfs.erofs`.
  - **Layer 5 — Flashable ZIP Assembly:** Generation of dynamic partition transfer lists and signed TWRP-flashable ZIP.
* **Cascade Behavior:** If an APK in Layer 2 is modified, Layers 0 and 1 remain frozen, and only the affected partition image is rebuilt in Layer 4. If `vendor` files were untouched, `vendor.img` is reused intact without wasting CPU cycles on EROFS or Brotli compression.

---

### FEATURE 4: Universal Cryptographic Key Management
* **Mechanism:**  
  Carbon Engine functions as a plug-and-play signing authority:
  1. **Automatic Private Key Discovery:** Searches `security/keys/platform.pk8` and `security/keys/platform.x509.pem`. If found, signs system apps and sets `ro.build.tags=release-keys`.
  2. **Secure AOSP Test-Keys Fallback:** If no private keys are supplied, defaults to open AOSP test keys, logging: `[NOTICE] Building with public AOSP test-keys`.
  3. **Dynamic PackageManager Injection (`services.jar`):** Extracts the active certificate and injects the platform signature into `InstallPackageHelper.smali`, ensuring system apps gain `system` (UID 1000) privileges.
  4. **Key Generation Assistant (`--generate-keys`):** Generates compliant RSA 4096-bit Android signing keys via a single command for new maintainers.
  5. **Hardened Secret Protection (`.gitignore`):** Enforces that private `.pk8`, `.key`, and `.keystore` files are strictly excluded from public Git repositories.

---

### FEATURE 5: Parallel I/O & CPU Scheduling
* **Mechanism:**  
  On multi-threaded architectures (such as 28-thread Xeon processors):
  - Dispatches parallel `apktool` jobs throttled by worker pools to prevent JVM Heap exhaustion (OOM).
  - Partition image building runs concurrently across dedicated threads.
  - Heavy Brotli compression is parallelized to eliminate NVMe/SSD storage bottlenecks.

---

### FEATURE 6: Atomic Transaction Commit Pattern
* **Mechanism:**  
  Prevents process interruptions from corrupting workspace trees:
  - Generated files are written to staging areas (`.tmp`).
  - Output checksums are validated against the compilation process.
  - Only on exit code `0`, an atomic Linux `rename()` syscall promotes the artifact to its final destination (`work_dir/...`).
  - The cache ledger is updated only after successful promotion.

---

### FEATURE 7: Forensic Build Ledger (`build_manifest.json`)
* **Mechanism:**  
  Every compilation generates structured telemetry containing:
  - Exact ISO 8601 build timestamp.
  - Component table: Name, Status (`CACHE_HIT` / `REBUILT`), input/output fingerprints, execution duration.
  - Time saved summary: *"Total time: 1m12s. Time saved via cache: 23m37s"*.
  - Injected APK and native library manifest with SHA-256 hashes for security auditing.

---

### FEATURE 8: CLI Flags and Environment Tracking
* **Mechanism:**  
  Toggling CLI options (such as `--no-telemetry-debloat`) invalidates the input hash of affected nodes automatically, ensuring rules take effect immediately even if underlying scripts were not modified on disk.

---

### FEATURE 9: Shared Framework Invalidation Guard
* **Mechanism:**  
  The engine tracks public framework resource IDs (`1.apk` / `framework-res.apk`). If a patch adds or alters public framework resources, all dependent APKs (`SecSettings`, `SystemUI`) are automatically marked for clean recompilation, preventing `aapt2 resource linking failed` errors.

---

### FEATURE 10: Standardized Distribution Artifacts
* **Naming Standards:**
  - **Package Name:** `UN1CA_Carbon_IX_1.0_<device>_<pda>.zip` (e.g., `UN1CA_Carbon_IX_1.0_dm1q_ZZI8.zip`).
  - **Build Display ID (`ro.build.display.id`):** `UN1CA Carbon IX v1.0 (<pda>)` (concise, single-line format).

---

## 4. ENGINE EXECUTION LIFECYCLE

```
[1. START] ──► Parse CLI Arguments and Environment Variables
      │
[2. DISCOVERY] ──► Scan security/keys/ and determine active profile (Release vs Test)
      │
[3. STAT-SCAN] ──► Compare mtime/size of all input folders against SQLite Ledger
      │
[4. FINGERPRINT] ──► Calculate Composite SHA-256 for modified nodes only
      │
[5. DAG RESOLUTION] ──► Build execution DAG:
      │                 - Unchanged nodes: Promote from Cache in 0.01s
      │                 - Modified nodes: Enqueue to parallel worker pool
      │
[6. PARALLEL EXEC] ──► Execute Compilations (Apktool / Smali / Patches) with atomicity
      │
[7. FS COMPOSER] ──► Build partition images with modified contents (mkfs.erofs)
      │
[8. PACKAGE & SIGN] ──► Assemble Flashable ZIP + Include TWRP recovery.img + Sign Package
      │
[9. LEDGER UPDATE] ──► Record new run metrics in ledger.db & build_manifest.json
      │
[10. FINISH] ──► Output audit telemetry and deliver final ZIP package
```

---

## 5. TECHNICAL INVARIANTS & INTEGRATION REQUIREMENTS

1. **Storage Cache Database:** Native SQLite with WAL mode (`out/.carbon_cache/ledger.db`).
2. **Native C++ Interfaces:**
   - `FingerprintEngine`: Directory and file hashing with exclusion masks (`.git`, `.bak`, `.orig`).
   - `ProcessSupervisor`: Subprocess lifecycle management with async stdout/stderr streaming and timeouts.
   - `ArtifactStore`: Content-addressed file store with SHA-256 indexing.
3. **Legacy UN1CA Compatibility:**
   - Existing modules and patches continue to operate seamlessly without requiring rewrites of hundreds of existing `.sh` and `.smali` scripts.

---

## 6. METRICS & ACCEPTANCE TARGETS

| Metric | Legacy Pipeline | Target with Carbon Engine |
| :--- | :--- | :--- |
| **Clean Full Build Time** | ~25 minutes | ~3 to 4 minutes (28-thread parallelization) |
| **Incremental Build (1 string changed)** | ~25 minutes | **< 60 seconds** |
| **Incremental Build (Kernel / Bootanim)** | ~25 minutes | **< 30 seconds** |
| **CPU/IO Redundancy on Unchanged Files** | 100% reprocessing | **0% (100% Cache HIT)** |
| **Cryptographic Traceability** | None (opaque) | **Detailed audit in `build_manifest.json`** |

---
*Official architecture master plan for ALRI Development (ALRIGROUP) and the UN1CA Carbon open-source initiative.*
