# 👑 UN1CA Carbon IX (v1.1) — Official Release Notes

**Target Device:** Samsung Galaxy S23 Base Universal (`SM-S911B` & `SM-S9110` / `dm1q`)  
**OS Version:** One UI 9.0 Stable (Android 17 • SDK 37)  
**Base Firmware:** `S911BXXUAGZIF` (Official Production Base)  
**Firmware Bootloader Bit:** **FIRMWARE BIT A** (Binary A / SW REV A)  
**Kernel:** GoRhanHee Linux 5.15.216 + KernelSU-Next v3.4.2 + SUSFS  
**Recovery:** Official TWRP 3.7.1_12-0 (`dm1q`) integrated  
**Release Tag:** `IX-v1.1`  
**Signature Profile:** `OFFICIAL_CARBON_KEYS` (RSA 4096-bit ALRI GROUP)  
**Security Patch:** 2026-10-05  

---

## 🚀 Complete Changelog: What's New from v1.0 to v1.1

### 1. 🌐 Base Firmware & Multi-Variant Hardware
* **Official Stable Firmware Promotion:** Promoted base from initial beta `ZZI8` to Samsung's official production firmware **`S911BXXUAGZIF`** (Firmware Bit A / SW REV A).
* **Universal Galaxy S23 Base Support:** Unified installer package compatible with European/Global **`SM-S911B`** and Chinese/Hong Kong/Taiwanese **`SM-S9110`** devices.
* **Dynamic Hardware Initialization:** Auto-detects physical hardware variants at boot (`unica_hardware_variant.rc`), assigning Dual SIM (`dsds`) to S9110 while preserving eSIM on S911B.

### 2. 🔐 Security, Cryptography & Enterprise Signing
* **Official ALRI GROUP RSA-4096 Keyset:** Production packages and partitions are now signed using our corporate 4-tier RSA 4096-bit keyset (`platform`, `releasekey`, `media`, `shared`).
* **Hardware-Level Signature Masking:** Custom framework hooks in `services.jar` allowing system-privileged applications to execute without signature mismatch rejections while protecting setup-wizard integrity.
* **Knox Matrix & Session Desynchronization Fix:** Permanently purged Samsung's `KmxService` and `fabric_crypto` background daemons, stopping Samsung Account automatic logout and session invalidation loops on unlocked bootloaders (Knox 0x1).

### 3. 🎛️ Carbon Performance Engine & Hardware Tuning
* **Native One UI Performance Dropdown:** Built directly into the primary `SecSettings.apk` dashboard:
  * 🍃 **Light Mode:** Clamps Prime Cortex-X3 to 1.97 GHz and Adreno 740 GPU to 348 MHz for maximum battery preservation and ice-cold thermals.
  * ⚖️ **Normal Mode:** Factory stock Snapdragon 8 Gen 2 for Galaxy frequencies.
  * 🔥 **Performance Mode:** Eliminates governor ramp-up latency (`up_rate_limit_us=0`), enables aggressive Adreno 740 clock floors, and boosts UFS read-ahead I/O to 512 KB.
* **8 GB ZRAM with Native ZSTD Compression:** Upgraded swap subsystem from legacy `lzo-rle` to high-speed **ZSTD** compression with `swappiness=100`, eliminating background application reloads.
* **Advanced GPU Tuning (Adreno 740):** Integrated KonaBess C++ native worker (`/system/bin/unica-gpu`) with live settings interface, automatic TWRP rollback safety partition, and dynamic FDT/DTB voltage curve adaptation.

### 4. 🧠 Local AI & Neural Framework (Qwen 2.5 LLM)
* **On-Device Offline Intelligence:** Integrated Qwen 2.5 1.5B/7B GGUF model framework running via `librezoss_llama` engine service (`io.mesalabs.rezoss.qwen`).
* **Settings Management Dashboard:** Dedicated Settings menu with real-time model status, hardware acceleration metrics (Snapdragon 8 Gen 2 Hexagon NPU + Adreno 740), and direct model download trigger.
* **Galaxy AI Writing Toolkit & SmartReply:** Enabled Samsung Keyboard (`HoneyBoard`) writing assistant capabilities, conversational tone adjustments, and Smart Suggestions for WhatsApp, Telegram, and Samsung Messages.

### 5. 📞 Audio Subsystem & Call Screening
* **Bixby Text Call Telephony Routing:** Resolved Qualcomm audio policy routing blocks by authorizing `AUDIO_CHANNEL_OUT_MONO` and 32 kHz sampling rates on `incall_music_uplink` and `voice_tx`.
* **Qualcomm PAL Graph Integration:** Injected `PAL_STREAM_VOICE_CALL_MUSIC` with `icmd_plus` custom config into `usecaseKvManager.xml`.
* **Audio Eraser & SoundAlive 2025:** Added neural voice/noise separation engines (`libmultisourceseparator.so`) and updated Galaxy 2025 acoustic audio profile.
* **SmartView & Wireless DeX HDCP Bypass:** Permanent bypass (`wlan.wfd.hdcp=disable`) enabling wireless screen projection to Smart TVs and PCs.

### 6. 🛠️ Build Engine & Pipeline Infrastructure
* **Carbon Engine C++20:** Replaced monolithic bash build scripts with a native C++20 build orchestrator featuring input-addressed DAG caching.
* **Cryptographic Forensic Manifest:** Every build now generates `out/build_manifest.json` cataloging deep SHA-256 hashes of every module, component, and recompiled APK/JAR.
* **Build Date Precision:** Fixed the legacy NPL "Unknown" build date bug in Settings by reading authentic system build timestamps (`ro.build.date`).
