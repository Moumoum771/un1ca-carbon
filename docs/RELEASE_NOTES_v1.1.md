# 👑 UN1CA Carbon IX (v1.1) — Release Notes

**Target Device:** Samsung Galaxy S23 Base Universal (`SM-S911B` & `SM-S9110` / `dm1q`)  
**OS Version:** One UI 9.0 Stable (Android 17 • SDK 37)  
**Base Firmware:** `S911BXXUAGZIF`  
**Firmware Bootloader Bit:** **FIRMWARE BIT A** (Binary A / SW REV A)  
**Kernel:** GoRhanHee Linux 5.15.216 + KernelSU-Next v3.4.2 + SUSFS  
**Recovery:** Official TWRP 3.7.1_12-0 (`dm1q`) integrated  
**Release Tag:** `IX-v1.1`  
**Date:** October 2026  
**License:** ARGLP (ALRI Group License Permissive) & Upstream GPL-3.0  

---

## 📥 Official Download Links & Checksums

* **Package Filename:** `UN1CA_Carbon_IX_1.1_dm1q_GZIF.zip`
* **Release Branch:** `carbon-ix-V1.0` (Production Tag `IX-v1.1`)

---

## 🚀 What's New in v1.1 (One UI 9.0 Stable Milestone)

1. **🌐 Universal S23 Base Hardware Support (`SM-S9110` + `SM-S911B`):**
   * Multi-variant recovery assert allowing installation across international `SM-S911B` and Hong Kong / Taiwan / China `SM-S9110`.
   * Runtime hardware auto-detection (`unica_hardware_variant.rc`) dynamically configuring Dual Physical SIM (`dsds`) on S9110 while preserving eSIM on S911B.

2. **🛡️ Native C++ Play Integrity Engine (`libunica_pif.so`):**
   * Private process memory remapping (`mmap`) hooked into `ZygoteInit` specifically for `com.google.android.gms.unstable`.
   * Passes Play Integrity device verdicts natively without polluting global system properties.

3. **🔒 Permanent Knox Matrix & Session Sign-Out Fix:**
   * Purged `KmxService` and `fabric_crypto` daemons, permanently stopping Samsung Account automatic logout/session revocations on unlocked bootloaders.

4. **⚡ Hardware Display & AMOLED Protection:**
   * Removed experimental S24 Ultra doze auto-brightness tables on `dm1q`, restoring stock Samsung display calibration and protecting against green tint and panel PWM stress.

5. **🧹 Build Pipeline Hygiene:**
   * Integrated `rom_payload.py` quarantining `.bak` files from image generation and OTA packaging.

---

## 📲 Clean Installation Guide (TWRP Recovery)

1. **Boot into Recovery:** Power off your Galaxy S23 and boot into TWRP Recovery.
2. **Format Userdata (Crucial):** Navigate to **Wipe > Format Data**, type `yes`, and confirm.  
   *(Required on clean installations to create authentic FBE metadata structures).*
3. **Transfer the ROM Package:**
   - Connect your phone to your PC via USB and copy `UN1CA_Carbon_IX_1.1_dm1q_GZIF.zip` to `/sdcard/`, or
   - Use USB-OTG flash drive, or
   - Use `adb sideload UN1CA_Carbon_IX_1.1_dm1q_GZIF.zip`.
4. **Flash the ROM:** Go to **Install**, select `UN1CA_Carbon_IX_1.1_dm1q_GZIF.zip`, and swipe to confirm flash.
5. **Reboot to System:** Tap **Reboot System**. Enjoy **UN1CA Carbon IX v1.1**!
