# UN1CA Carbon — Modular Telemetry Debloater Specification

**Maintainer:** AlexsAR (@alexsanderalri)  
**Port & Engineering:** AlexsanderALRI  
**Project:** UN1CA Carbon (Version: IX • One UI 9.0 • Android 17 • SDK 37)  
**Target:** Samsung Galaxy S23 Series (`dm1q`, `dm2q`, `dm3q` / Snapdragon 8 Gen 2 / SM8550)  
**Module Identifier:** `telemetry_debloat`  
**Configuration Variable:** `DEBLOAT_TELEMETRY` (Default: `true`)

---

## 1. Executive Summary & Epistemic Objectives

The **UN1CA Carbon Telemetry Debloater** is an enterprise-grade, configurable debloating subsystem designed to eliminate background surveillance daemons, behavioral profiling engines, diagnostic reporters, Qualcomm prototype code, and partner telemetry from Samsung One UI 9.0 builds.

### Primary Objectives:
1. **User Privacy & Zero Behavioral Profiling:** Strip out persistent background loggers (`RubinVersion38`, `DiagMonAgent97`, `knoxanalyticsagent`) that record app interactions, cellular towers, modem crash logs, and user metadata for upload to OEM servers.
2. **Resource Footprint & Battery Longevity:** Terminate wakelock-heavy polling daemons (`DeviceQualityAgent38`, `SilentLog`) that write telemetry dumps directly into internal UFS flash storage.
3. **Firmware Hygiene & Prototype Pruning:** Remove dead vendor prototype demonstrations from Qualcomm Snapdragon BSP (`QaiorScreenUnderstandingService`, `screenunderstanding`) and third-party advertising hooks (`HiyaService`, `SmartManager_v6_DeviceSecurity_CN`, `WarrantyCare`).
4. **Configurable & Fully Reversible:** Controlled via a single centralized build variable (`DEBLOAT_TELEMETRY`). Can be cleanly toggled to `false` at build time without breaking partition balance, SELinux policies, or system dependencies.

---

## 2. Configuration & Build Controls

The debloater is governed by the environment variable:

```bash
export DEBLOAT_TELEMETRY=true   # Default: removes all 11 telemetry packages
export DEBLOAT_TELEMETRY=false  # Preserves OEM telemetry, diagnostics, and prototypes
```

### Supported Activation Methods:

#### Method A: Environment Variable via Build Orchestrator
```bash
# Default build (telemetry debloated):
./make_carbon.sh dm1q

# Build preserving telemetry:
DEBLOAT_TELEMETRY=false ./make_carbon.sh dm1q
```

#### Method B: Command-Line Flags
The build orchestrator (`make_carbon.sh`) and environment bootstrap (`core_unica/buildenv.sh`) provide native flags:

```bash
# Disable debloat via CLI flag:
./make_carbon.sh dm1q --no-telemetry-debloat

# Force enable debloat via CLI flag:
./make_carbon.sh dm1q --telemetry-debloat
```

#### Method C: Interactive Shell & Buildenv
When sourcing the environment manually:
```bash
source core_unica/buildenv.sh --no-telemetry-debloat dm1q
```

The configuration is captured during config generation in `scripts/internal/gen_config_file.sh` via `GET_BUILD_VAR "DEBLOAT_TELEMETRY" "true"` and saved into `$OUT_DIR/config.sh`.

---

## 3. Package Inventory & Debloat Scope

When `DEBLOAT_TELEMETRY=true`, the following **11 packages** are excised from the target rootfs:

| # | Partition Path | Package Name | Common Label | Operational Impact & Debloat Rationale |
|---|----------------|--------------|--------------|-----------------------------------------|
| 1 | `system/priv-app/RubinVersion38` | `com.samsung.android.rubin.app` | Customization Service | Behavioral tracking engine that monitors app usage, geographic habits, and interaction patterns to deliver targeted ads across One UI. Completely safe to remove; eliminates background wakeups. |
| 2 | `system/priv-app/DiagMonAgent97` | `com.sec.android.diagmonagent` | DiagMonAgent | Diagnostic monitor daemon uploading crash dumps, memory allocations, and error metrics to Samsung analytics servers. Safe to delete. |
| 3 | `system/priv-app/DeviceQualityAgent38` | `com.samsung.android.dqa` / `dqagent` | DQA | Background watchdog telemetry tracking Wi-Fi connection anomalies and hardware stress states. Safe to delete. |
| 4 | `system/priv-app/knoxanalyticsagent` | `com.samsung.android.knox.analytics.agent` | Knox Analytics Uploader | Aggregates Knox container analytics and security events, uploading telemetry batches to Samsung Cloud. Non-critical; safe to delete. |
| 5 | `system/app/SilentLog` | `com.sec.modem.settings` | SilentLogging | Diagnostic daemon capturing modem logs, baseband states, and cellular signaling into flash storage. Unneeded outside lab debugging; saves UFS flash wear. |
| 6 | `system/app/SmartManager_v6_DeviceSecurity_CN` | `com.samsung.android.sm.devicesecurity.tcm` | App protection (TCM) | Chinese market anti-virus engine by Tencent/360 integrated into Device Care. Partner bloatware; safe to delete. |
| 7 | `system/app/HiyaService` | `com.hiya.star` / `com.samsung.android.app.hiya` | Hiya Service | Third-party spam caller lookup that transmits incoming call numbers to Hiya cloud endpoints. Safe to delete. |
| 8 | `system/priv-app/WarrantyCare` | `com.samsung.android.warrantycare` | Warranty and care | Nag service prompting users for device warranty registration and Samsung Care+ insurance purchases. Marketing bloatware; safe to delete. |
| 9 | `system_ext/priv-app/QaiorScreenUnderstandingService` | `com.qualcomm.qaior.screen_understanding` | ScreenUnderstanding | Qualcomm AI on Radio demonstration service for multimodal screen analysis. Non-production prototype code; safe to delete. |
| 10 | `system_ext/priv-app/screenunderstanding` | `com.qualcomm.qaior.screen_understanding.sample_app` | QAIOR Screen Understanding Sample App | Companion prototype test app for Qualcomm's screen understanding service. Dead bloat; safe to delete. |
| 11 | `product/priv-app/CarrierWifi` | `com.google.android.apps.carrier.carrierwifi` | Carrier Wi-Fi | Google carrier Wi-Fi provisioner connecting to partner carrier public Wi-Fi hotspots automatically. Safe to delete. |

---

## 4. Architectural Implementation

### 4.1 Module Structure
The module is located in `carbon_ours/telemetry_debloat/` and symlinked into `unica/mods/telemetry_debloat` by `make_carbon.sh`:

```
carbon_ours/telemetry_debloat/
├── module.prop      # UN1CA module declaration
└── customize.sh     # Modular execution and prune script
```

#### `module.prop`
```properties
id=telemetry_debloat
name=UN1CA Carbon Telemetry Debloater
author=AlexsAR
description=Modular, configurable debloater for OEM diagnostics, tracking, analytics, and telemetry packages controlled by DEBLOAT_TELEMETRY.
```

### 4.2 Module Ordering Advantage
In UN1CA, mods inside `unica/mods/` are processed in alphabetical order by `core_unica/scripts/internal/apply_modules.sh`:
1. `cosine` (`vendors_npl/mods_cosine`)
2. `hma` (`vendors_rezoss/mods_hma`)
3. `rezoss` (`vendors_rezoss/mods_rezoss`)
4. `settings` (`carbon_ours/branding_settings`)
5. **`telemetry_debloat`** (`carbon_ours/telemetry_debloat`)

Because `telemetry_debloat` executes after `rezoss`, it cleanly purges `SmartManager_v6_DeviceSecurity_CN` even though `vendors_rezoss` supplies it as a prebuilt package.

### 4.3 Dual-Layer Pruning Mechanism
`customize.sh` employs a resilient two-tier deletion pipeline:
1. **UN1CA Engine Pruning (`DELETE_FROM_WORK_DIR`):**  
   Prunes the package directory and strips its file entries from `configs/fs_config-<partition>` and `configs/file_context-<partition>`. This ensures the filesystem generator does not fail on missing inodes or dangling SELinux contexts.
2. **Defensive Filesystem Sweep (`rm -rf`):**  
   Ensures absolute cleanup across both flat (`$WORK_DIR/system/app`) and system-as-root (`$WORK_DIR/system/system/app`) path topologies, as well as dynamic `system_ext` partition structures.

---

## 5. Multi-Repository Mirroring & Synchronization

To maintain full compatibility between the standalone `Un1ca_Carbon` repository and the upstream development tree `ALRI_Un1ca`, the debloat rules are mirrored across:

1. **`Un1ca_Carbon/carbon_ours/telemetry_debloat/customize.sh`**: Core modular debloat script.
2. **`Un1ca_Carbon/carbon_ours/target_dm1q/debloat.sh`**: Target-level debloat definitions for Galaxy S23 (`dm1q`).
3. **`ALRI_Un1ca/target/dm1q/debloat.sh`**: Mirrored Galaxy S23 debloat list with `DEBLOAT_TELEMETRY` guard.
4. **`ALRI_Un1ca/unica/debloat.sh`**: Global debloat list with `DEBLOAT_TELEMETRY` guard.
5. **`ALRI_Un1ca/unica/configs/version.sh`** & **`buildenv.sh`**: Variable export and CLI toggle support.

---

## 6. Verification and Acceptance Testing

### Verification Checklist:
- [x] `bash -n` syntax check passed for all modified and created shell scripts.
- [x] Verified default variable `DEBLOAT_TELEMETRY=true` in `make_carbon.sh`, `core_unica/buildenv.sh`, and `configs/version.sh`.
- [x] Verified CLI flag override (`--no-telemetry-debloat`) successfully flips `DEBLOAT_TELEMETRY=false`.
- [x] Verified `module.prop` adheres to UN1CA format.
- [x] Verified `customize.sh` idempotency (safe to run multiple times without failure).
- [x] Verified clean excision of all 11 targeted APK directories from `$WORK_DIR`.
- [x] Mirrored configuration in `ALRI_Un1ca/target/dm1q/debloat.sh` and `ALRI_Un1ca/unica/debloat.sh`.
