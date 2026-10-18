# 📊 TECHNICAL ENGINEERING REPORT: ARCHITECTURE, INTEGRATION & ROOT-CAUSE RESOLUTION
**Project:** UN1CA Carbon — Samsung One UI 9.0 (Android 17 / SDK 37)  
**Target Device:** Samsung Galaxy S23 (`SM-S911B` / `dm1q` / Qualcomm Snapdragon 8 Gen 2)  
**Base Firmware:** `S911BXXUAZZI8` (One UI 9 Beta 2)  
**Final Status:** ✅ **100% Functional Boot, Stable Display Compositor, Official Keyboard Active**

---

## 🏛️ 1. Upstream Lineage & Repository Fusion

The construction of the unified **`UN1CA Carbon`** tree executed a strategic architectural fusion of three distinct codebases:

```
                  ┌────────────────────────────────────────┐
                  │      salvogiangri/UN1CA (sixteen)      │
                  │   Core Architecture & Dynamic Partitions│
                  └───────────────────┬────────────────────┘
                                      │
        ┌─────────────────────────────┴─────────────────────────────┐
        ▼                                                           ▼
┌─────────────────────────────────┐               ┌─────────────────────────────────┐
│   Rezoss-Reza/UN1CA (sembilan)  │               │ cosinemyan/NPLROM_UN1CA (npl-9) │
│ One UI 9 Engine / S23U Framework│               │   Visual Assets & Wallpapers    │
└───────────────┬─────────────────┘               └────────────────┬────────────────┘
                │                                                  │
                └─────────────────────┬────────────────────────────┘
                                      ▼
                      ┌───────────────────────────────┐
                      │    UN1CA Carbon (alri-oneui9) │
                      │   Stable S23 Base Engineering │
                      └───────────────────────────────┘
```

### A. From UN1CA Upstream (`salvogiangri/UN1CA:sixteen`)
* **Modular Build Pipeline:** Orchestration scripts (`buildenv.sh`, `make_rom.sh`, `scripts/build_fs_image.sh`), unsparse handling with `simg2img`, filesystem compilation via `erofs`, and block-level payload compression with `brotli`.
* **Hardware Profile Mapping (`target/dm1q/config.sh`):** Physical partition structure and block definitions for the Snapdragon 8 Gen 2 platform (`sm8550`).
* **Core Customization Modules:**
  * `deknox`: Purges redundant Knox telemetries and enterprise daemons.
  * `knoxpatch`: Emulates Knox integrity verification APIs.
  * `games-fps-unlocker`: Injects `GamesPropsUtils` for high refresh rate gaming (120 FPS).
  * `pif`: Injects property overrides for Play Integrity attestation.

### B. From Rezoss-Reza (`Rezoss-Reza/UN1CA:sembilan`)
The pioneering base porting One UI 9 to Snapdragon 8 Gen 2 on the S23 Ultra:
* **4-Certificate Signature Verification Bypass (`services.jar`):** Hook in `PackageManagerService` allowing custom system package installation without trigger bootloader signature halts.
* **HMA OSS (Hide My Applist):** System-level privacy hooks in `framework.jar` restricting app visibility at the Binder IPC boundary.
* **TEE-Simulator:** Emulates Trusted Execution Environment hardware calls, preventing TrustZone panics.
* **DFE Auto-Detection (Disable Force Encryption):** Detects storage encryption state during flash routines.
* **Battery & Lockscreen Enhancements:** Maximum power-saving background throttling and custom font typography mirroring between lockscreen clock and Always On Display.

### C. From NPL ROM (`cosinemyan/NPLROM_UN1CA:npl-oneui9`)
* **Visual Identity & Theming:** The `cosine` module delivering high-resolution Sagarmatha Himalayan wallpapers (`wallpaper-res.apk`).
* **ThemeCenter Trial Bypass:** The `inject_trial_gate.py` script disabling the 10-minute trial expiration on Galaxy Store themes.
* **Firmware Deduplication:** Optimizations in `download_fw.sh` preventing redundant local firmware downloads.

---

## ✂️ 2. Pruned, Replaced, and Calibrated Components

To transform an experimental S23 Ultra port into a rock-solid, production-grade daily driver for the Galaxy S23 Base (`dm1q`), the following interventions were engineered:

| Component | Upstream Origin | Action Taken | Technical Rationale |
| :--- | :--- | :--- | :--- |
| **`libframebooster.so`** | Rezoss (`m3qxxx`) | **Pruned & Restored Stock S23** | S26 Ultra prebuilt binary imported by upstream required symbol `fmt::v11`. Native S23 runtime provides `fmt::v12`. The mismatch triggered fatal dynamic linker aborts (`CANNOT LINK EXECUTABLE`) on `surfaceflinger`. |
| **`libmpp*.so` Family** | Rezoss (`m3qxxx`) | **Pruned & Restored Stock S23** | S26 Ultra media wrapper modified method signature for `renderAndReleaseOutputBuffer` in `MppWrapper`, producing an unhandled `UnsatisfiedLinkError` in `SystemUI` (`libmediasndk.so`). |
| **`HoneyBoard.apk` (Keyboard)**| Rezoss | **Pruned Smali Patches & Restored Stock** | Upstream Now Nudge patches forced APK recompilation and testkey signing. Under factory-reset FRP mode, Android 17 strictly forbids system input methods lacking official Samsung signatures (`CN=Samsung Cert`). |
| **`DYN_RESOLUTION_CONTROL`** | Rezoss / S23U | **Disabled (`false`)** | The Galaxy S23 base panel is a fixed FHD+ (1080x2340) display. Enforcing WQHD+ dynamic resolution control caused graphics buffer allocation failures. |
| **Display Mode (`HFR_MODE`)** | Config dm1q | **Changed from 3 to 2** | S23 base utilizes an adaptive 24Hz–120Hz mode (Mode 2). Mode 3 is specific to S23 Ultra LTPO 1Hz–120Hz panels. |
| **Super Partition Size** | Config dm1q | **Set to `12266242048` Bytes** | Perfectly aligned with the physical UFS storage block boundary of the base model (`dm1q`). |
| **`KnoxGuard.apk` & `kgclient`**| Upstream UN1CA | **Preserved Intact** | The TrustZone enclave (`tz_kg`) permanently locks device hardware upon detecting the absence of `KnoxGuard.apk` in `/system/priv-app/`. |
| **Kernel Partitions in OTA** | Upstream / NPL | **Kernel Flash Gated** | Preserved stock signed kernel partitions (`boot`, `init_boot`, `dtbo`, `vendor_boot`) against Qualcomm ABL verified boot state rejections. |
| **Galaxy Store Downloads** | Rezoss Modules | **Defensive Fallback Added** | Upstream scripts halted compilation when Galaxy Store APIs returned null URLs for SDK 37 packages (`KmxService`, `GameTools`, `SBrowser`). Added automated fallback to stock firmware packages. |
| **`file_context-system_ext`** | Rezoss Modules | **SELinux Context Repaired** | Trailing space without security label on `MoseyApp.apk` caused `mkfs.erofs` compilation aborts. Default `u:object_r:system_file:s0` applied. |

---

## 💥 3. Root-Cause Analysis of Investigated Bootloops

### 🔴 Failure 1: SurfaceFlinger Linker Abort (Bootloop to Recovery)
* **Symptom:** The device vibrated on power-on, started kernel initialization, and abruptly rebooted back into TWRP Recovery after approximately 10 seconds.
* **Log Evidence (`/cache/recovery/rescueparty_log`):**
  ```text
  CANNOT LINK EXECUTABLE "/system/bin/surfaceflinger": 
  cannot locate symbol "_ZN3fmt3v117vformatENS0_17basic_string_viewIcEENS0_17basic_format_argsINS0_7contextEEE" 
  referenced by "/system/lib64/libframebooster.so"...
  ```
* **Root Cause:** Upstream modules replaced the native display optimization library with a prebuilt blob from the S26 Ultra (`m3qxxx`). This prebuilt binary was compiled against version 11 of the C++ `fmt` formatting library. However, the Galaxy S23 One UI 9 base firmware exports version 12 (`libbase.so`). The Linux dynamic linker aborted `surfaceflinger`. Following 5 consecutive crashes, Android's **RescueParty** forced an emergency reboot into Recovery.
* **Resolution:** Implemented the `_REZOSS_ADD_SYSTEM_LIB64` defensive guard to prevent overwriting native libraries that already exist in stock S23 firmware, restoring the untouched `libframebooster.so` exporting `fmt::v12`.

---

### 🔴 Failure 2: SystemUI & ImageWallpaper Crash Loop
* **Symptom:** AMOLED display flickering intermittently and lockscreen interface freezing.
* **Log Evidence (`/cache/recovery/rescueparty_log`):**
  ```text
  FATAL EXCEPTION: ImageWallpaper (Process: com.android.systemui)
  java.lang.UnsatisfiedLinkError: dlopen failed: 
  cannot locate symbol "_ZN7android10MppWrapper28renderAndReleaseOutputBufferEillRKNS_2spINS_8AMessageEEE" 
  referenced by "/system/lib64/libmediasndk.so"...
  ```
* **Root Cause:** Native One UI 9 video and wallpaper players attempted to invoke media buffer functions whose ABI had diverged in the S26 Ultra `libmpp.so` package.
* **Resolution:** Complete restoration of native Snapdragon 8 Gen 2 media libraries (`libmpp.so`, `libmpp_common.so`, `libmppclient.so`, `libmppfilter.so`, `vendor.samsung.hardware.media.mpp-V5-ndk.so`) extracted from the target S23 firmware.

---

### 🔴 Failure 3: Post-Android 17 TWRP Storage Mount Rejection
* **Symptom:** TWRP reported `Failed to mount '/data' (Invalid argument)` and could not mount `/sdcard`.
* **Root Cause:** During its initial boot sequence, the Android 17 runtime initializes updated File-Based Encryption (FBE) metadata structures within the F2FS filesystem on `/dev/block/sda41`. Because older TWRP recovery kernels lack backward-compatible keyring hooks for Android 17 superblock changes, mounting failed without a clean format.
* **Resolution:** Performed a clean **Format Data** operation within TWRP to reset the F2FS superblock, followed by the activation of native DFE (`encryptable` in `fstab.qcom`) to prevent re-encryption.

---

### 🔴 Failure 4: Samsung Keyboard Hidden During Setup Wizard
* **Symptom:** Device booted successfully to the One UI Setup Wizard, but the on-screen keyboard failed to render upon focusing password/PIN fields.
* **Root Cause:** Upstream patches applied smali modifications to `HoneyBoard.apk` for experimental Now Nudge features, which forced testkey re-signing (`CN=Android`). When operating under Factory Reset Protection (FRP), Android 17 enforces `USER_SETUP_RESTRICTED` security profiles, strictly rejecting any `InputMethodService` that lacks an authentic Samsung signature (`CN=Samsung Cert`).
* **Resolution:** Pruned upstream smali patches and restored the stock `HoneyBoard.apk` with its precompiled ahead-of-time bytecode (`oat/arm64/HoneyBoard.odex` and `.art`).

---

## 🚀 4. Current Device State & Operating Profile

Following these resolutions:
1. **Samsung One UI 9.0 (Android 17)** runs natively with high stability on the Galaxy S23 (`dm1q`).
2. **Display Compositor:** Renders at native 120Hz with zero flickering or frame drops.
3. **Samsung Keyboard:** Operates reliably with multi-language input and FRP compatibility.
4. **Bootloader & Critical Partitions:** Protected against AVB state rejections and soft-bricks.
