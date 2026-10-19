# 🛠️ Porting UN1CA Scripts to Carbon Engine

**UN1CA Carbon Release:** IX (One UI 9.0 • Android 17 • SDK 37)  
**Organization:** ALRI Development ([ALRIGROUP](https://github.com/alrigroup))  
**Authors:** Alexsander "AlexsAR" ALRI (`@alexsanderalri`), ALRI Systems Engineering Team  

---

## 1. Context & Motivation

Legacy UN1CA builds operated entirely through bash scripts:
* `make_carbon.sh` / `make_rom.sh`
* `scripts/internal/apply_modules.sh`
* `scripts/utils/module_utils.sh`
* Individual module scripts: `customize.sh` in each mod directory.

While this architecture was modular in design, porting it to **One UI 9.0 (Android 17 / SDK 37)** exposed severe limitations:
1. **Smali & Resource Duplication:** When an incremental build runs over an existing `work_dir`, naive `sed` or `patch` commands in `customize.sh` append duplicate XML colors, strings, or smali goto labels, triggering fatal AAPT2 and Smali compilation errors.
2. **Hidden Subshell Failures:** Running `eval` inside unquoted bash functions masks errors.
3. **Hardcoded Model Identifiers:** Scripts had `dm1q`, `SM-S911B`, and `qssi` hardcoded throughout, breaking universal builds for the S23 series (`dm2q`, `dm3q`).

To solve this while **strictly preserving all legacy scripts as working fallbacks**, Carbon Engine introduces a declarative abstraction layer (`Carbonfile.json`) backed by dynamic macro expansion and idempotent patching.

---

## 2. Legacy Scripts vs. Carbon Engine Declarative Actions

In legacy scripts, operations are executed imperatively through shell functions. In Carbon Engine, operations are defined declaratively as atomic JSON actions:

### 2.1. File System Copy & Permissions

**Legacy Bash (`customize.sh`):**
```bash
cp -a "$MODPATH/system/bin/bootanimation_zip" "$WORK_DIR/system/system/bin/bootanimation_zip"
SET_METADATA "system" "system/bin/bootanimation_zip" 0 2000 755 "u:object_r:bootanim_exec:s0"
```

**Carbon Engine (`Carbonfile.json`):**
```json
{
  "id": "bootanim_runner_bin_zip",
  "category": "Boot & Media",
  "description": "Install AOSP bootanimation runner binary",
  "action": "fs.copy",
  "source": "carbon_ours/bootanimation/system/bin/bootanimation_zip",
  "destination": "${WORK_DIR}/system/system/bin/bootanimation_zip",
  "metadata": {
    "uid": 0,
    "gid": 2000,
    "mode": "0755",
    "selabel": "u:object_r:bootanim_exec:s0"
  }
}
```

### 2.2. Android System Properties

**Legacy Bash (`customize.sh`):**
```bash
SET_PROP "system" "ro.build.display.id" "UN1CA Carbon IX v1.0 (ZZI8)"
SET_PROP "system" "persist.sys.unica.bootanim" "carbon"
```

**Carbon Engine (`Carbonfile.json`):**
```json
{
  "id": "carbon_display_id",
  "category": "Branding",
  "description": "Set clean one-line Carbon build display ID",
  "action": "prop.set",
  "target_file": "${WORK_DIR}/system/system/build.prop",
  "key": "ro.build.display.id",
  "value": "UN1CA Carbon IX v1.0 (${PDA_SUFFIX})"
}
```

### 2.3. Smali Method Injections

**Legacy Bash (`customize.sh`):**
```bash
SMALI_PATCH "system" "system/framework/framework.jar" \
    "smali/android/app/Instrumentation.smali" "replace" \
    'newApplication(Ljava/lang/Class;Landroid/content/Context;)Landroid/app/Application;' \
    'invoke-virtual {p0, p1}, Landroid/app/Application;->attach(Landroid/content/Context;)V' \
    '    invoke-virtual {p0, p1}, Landroid/app/Application;->attach(Landroid/content/Context;)V\n\n    invoke-static {p1}, Lio/mesalabs/unica/SamsungPropsHooks;->init(Landroid/content/Context;)V'
```

**Carbon Engine (`Carbonfile.json`):**
```json
{
  "id": "instrumentation_props_hook",
  "category": "Framework Hooks",
  "description": "Inject SamsungPropsHooks into Instrumentation.newApplication",
  "action": "patch.smali",
  "target_file": "${APKTOOL_DIR}/system/framework/framework.jar/smali/android/app/Instrumentation.smali",
  "method": "newApplication(Ljava/lang/Class;Landroid/content/Context;)Landroid/app/Application;",
  "pattern": "invoke-virtual {p0, p1}, Landroid/app/Application;->attach(Landroid/content/Context;)V",
  "replacement": "    invoke-virtual {p0, p1}, Landroid/app/Application;->attach(Landroid/content/Context;)V\n\n    invoke-static {p1}, Lio/mesalabs/unica/SamsungPropsHooks;->init(Landroid/content/Context;)V"
}
```

---

## 3. The Universal Multi-Target Macro System

To eliminate hardcoded device references, `RecipeExecutor` and `PipelineOrchestrator` implement dynamic macro substitution. When porting or writing an action, always use variables instead of hardcoded strings:

| Hardcoded Legacy String | Carbon Engine Macro | Example Resolution (`dm1q`) |
| :--- | :--- | :--- |
| `dm1q` | `${TARGET}` | `dm1q` |
| `SM-S911B` | `${TARGET_MODEL}` | `SM-S911B` |
| `out/target/dm1q/work_dir` | `${WORK_DIR}` | `/path/to/out/target/dm1q/work_dir` |
| `out/target/dm1q/apktool` | `${APKTOOL_DIR}` | `/path/to/out/target/dm1q/apktool` |
| `out/fw/SM-S911B_OXM` | `${FW_DIR}` | `/path/to/out/fw/SM-S911B_OXM` |
| `out/` | `${OUT_DIR}` | `/path/to/out` |

### How the Macro Resolver Operates in C++20:
In `carbon_engine/src/core/recipe_executor.cpp`:
```cpp
std::string RecipeExecutor::ResolveMacros(const std::string& input,
                                          const std::unordered_map<std::string, std::string>& extra_env) const {
    std::string result = input;
    auto replace_macro = [&](const std::string& key, const std::string& val) {
        std::string tag = "${" + key + "}";
        size_t pos = 0;
        while ((pos = result.find(tag, pos)) != std::string::npos) {
            result.replace(pos, tag.length(), val);
            pos += val.length();
        }
    };
    replace_macro("TARGET", target_device_);
    replace_macro("WORK_DIR", work_dir_.string());
    replace_macro("APKTOOL_DIR", apktool_dir_.string());
    replace_macro("FW_DIR", fw_dir_.string());
    replace_macro("OUT_DIR", out_dir_.string());
    for (const auto& [k, v] : extra_env) {
        replace_macro(k, v);
    }
    return result;
}
```

---

## 4. Idempotency Engineering: Eliminating Build Loop Errors

One of the most consequential bugs discovered during the migration to One UI 9 was **build non-idempotency**.

### 4.1. AAPT2 Duplicate Resources in `SecSettings.apk`
When `branding_settings/customize.sh` injected XML colors and strings into `SecSettings.apk`, running the build a second time caused duplicate `<color>` and `<string>` tags. AAPT2 halted with:
```
error: duplicate value for resource 'color/sec_dashboard_unica_color' with config ''.
error: resource previously defined here.
```
**The Idempotent Guard:**
```bash
if grep -q "sec_dashboard_unica_color" "$APKTOOL_DIR/system/priv-app/SecSettings/SecSettings.apk/res/values/colors.xml" 2>/dev/null; then
    LOG "- \"colors.xml\" already patched in SecSettings.apk, skipping..."
else
    # Apply patch safely
fi
```

### 4.2. Duplicate Smali Labels in `services.jar`
In `WindowManagerService.smali`, applying privacy display and screenshot patches consecutively inserted the `:cond_1` and `:cond_2` labels twice:
```
out/target/dm1q/apktool/system/framework/services.jar/.../WindowManagerService.smali[40934,4] There is already a label with that name.
```
**The Architectural Fix in Carbon Engine:**
1. **Fresh WorkDir Snapshots:** Stage 3 (`Stage3_InitWorkDir`) executes an atomic `rsync` from the extracted stock firmware base to wipe dirty patched files.
2. **Deterministic Patches:** Patches check if the signature already exists prior to applying diffs.

### 4.3. Idempotent `HEX_PATCH` in `core_unica/scripts/utils/module_utils.sh`
When patching binary shared libraries (e.g. `camera.qcom.so`, `libstagefright.so`), `HEX_PATCH` now checks if the target hex already contains the replacement string before rewriting bytes:
```bash
if grep -Fqa "$REPLACEMENT" "$TARGET_FILE"; then
    LOG "- Hex patch already present in $(basename "$TARGET_FILE"), skipping."
    return 0
fi
```

---

## 5. Step-by-Step: Adding a New Custom Mod to Carbon Engine

To register a new custom feature into Carbon Engine:

### Step 1: Create the Module Directory
Place your module under `carbon_ours/` (or `vendors_<name>/`):
```
carbon_ours/my_custom_feature/
├── module.prop
├── customize.sh
└── system/
    └── system/etc/my_config.json
```

### Step 2: Write `module.prop`
```properties
id=my_custom_feature
name=My Custom Feature
version=v1.0
versionCode=1
author=ALRI Development
description=Enables advanced neural engine optimizations for Galaxy S23.
```

### Step 3: Implement Idempotent `customize.sh`
```bash
#!/usr/bin/env bash
SKIPUNZIP=1

LOG_STEP_IN "- Applying My Custom Feature"

# Copy prebuilt files
mkdir -p "$WORK_DIR/system/system/etc"
cp -a "$MODPATH/system/system/etc/my_config.json" "$WORK_DIR/system/system/etc/my_config.json"
SET_METADATA "system" "system/etc/my_config.json" 0 0 644 "u:object_r:system_file:s0"

LOG_STEP_OUT
```

### Step 4: Register Action in `Carbonfile.json`
Add the declarative step into `carbon_engine/Carbonfile.json`:
```json
{
  "id": "my_custom_feature_config",
  "category": "Neural & System",
  "description": "Install custom neural optimization config",
  "credit": "Engineered by ALRI Development (@alexsanderalri)",
  "action": "fs.copy",
  "source": "carbon_ours/my_custom_feature/system/system/etc/my_config.json",
  "destination": "${WORK_DIR}/system/system/etc/my_config.json",
  "metadata": {
    "uid": 0,
    "gid": 0,
    "mode": "0644",
    "selabel": "u:object_r:system_file:s0"
  }
}
```

The step will be automatically parsed, indexed by array order in the DAG, cached in `ledger.db`, and tracked during every build.
