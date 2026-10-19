# 🎨 Boot Animation & Display Graphics Subsystem

**UN1CA Carbon Release:** IX (One UI 9.0 • Android 17 • SDK 37)  
**Organization:** ALRI Development ([ALRIGROUP](https://github.com/alrigroup))  
**Authors:** Alexsander "AlexsAR" ALRI (`@alexsanderalri`), ALRI Systems Engineering Team  
**Hardware Target:** Samsung Galaxy S23 Series (Snapdragon 8 Gen 2 • Adreno 740)  

---

## 1. Architectural Overview

In stock Samsung One UI firmware, boot animation rendering is handled by a proprietary closed-source binary located at `/system/bin/bootanimation`. This binary loads Samsung-specific `.qmg` container files (`bootsamsung.qmg` and `bootsamsungloop.qmg`) using Qualcomm proprietary display hooks and Samsung audio daemons.

**UN1CA Carbon** completely bypasses the legacy Samsung QMG pipeline, establishing an authentic, high-performance **AOSP Frame-0 Boot Animation** pipeline that supports dynamic runtime switching between two curated experiences:
1. **UN1CA Carbon (Default):** Custom 24 FPS introductory sequence (`part0/`) and an infinite breathing pulse loop (`part1/`).
2. **Android 17 (REZOSS):** Native AOSP glowing Android 17 logo rendered directly through OpenGL ES / BLASTBufferQueue.

```
                  [SurfaceFlinger Boot Event]
                              │
                              ▼
                  [service bootanim started]
                              │
                              ▼
                 /system/bin/bootanimation
               (Native AOSP 64-bit Binary)
                              │
               ┌──────────────┴──────────────┐
               ▼                             ▼
       [bootanimation.zip]          [/dev/null Masked]
        (UN1CA Carbon Mode)        (Android 17 REZOSS Mode)
               │                             │
               ▼                             ▼
      Plays Custom Frames           Falls back to Native
      (1080x2340 @ 120Hz)           Glowing Android Logo
```

---

## 2. Eliminating the Samsung QMG Engine

In Samsung stock firmware:
* `/system/bin/bootanimation` (77 KB) dynamically links against `libQmageDecoder.so`, `libsecaudioinfo.so`, and `libtinyalsa.so`.
* It expects `bootsamsung.qmg` in `/system/media/`.

In UN1CA Carbon:
1. `/system/bin/bootanimation` is directly replaced with the standalone 64-bit AOSP player binary (`bootanimation_zip`, 51 KB).
2. The proprietary files `bootsamsung.qmg` and `bootsamsungloop.qmg` are deleted during build time (`rm -f $WORK_DIR/system/system/media/bootsamsung*.qmg`).
3. When `SurfaceFlinger` triggers `ctl.start bootanim`, it executes the AOSP binary on **Frame 0**, completely preventing Samsung's logo from flashing on screen.

---

## 3. Dynamic Switcher Architecture

The boot animation is configurable at runtime from the One UI Settings app:
**Settings > UN1CA Settings > Interface > Boot Animation**
* Options: `UN1CA Carbon` (value: `carbon`) or `Android 17 (REZOSS)` (value: `rezoss`).
* Property: `persist.sys.unica.bootanim`

### 3.1. Settings UI Controller (`BootAnimationPreferenceController.smali`)
The preference controller is implemented in smali within `SecSettings.apk`:
```smali
.method public onPreferenceChange(Landroidx/preference/Preference;Ljava/lang/Object;)Z
    .locals 2

    check-cast p2, Ljava/lang/String;

    # Writes "carbon" or "rezoss" to system properties
    const-string p1, "persist.sys.unica.bootanim"
    invoke-static {p1, p2}, Landroid/os/SemSystemProperties;->set(Ljava/lang/String;Ljava/lang/String;)V

    const/4 p0, 0x1
    return p0
.end method
```

### 3.2. Runtime Controller Script (`rezoss_bootanimation.sh`)
When the system initializes during `post-fs-data`, `/system/etc/unica/rezoss_bootanimation.sh` evaluates the property:

```bash
#!/system/bin/sh
#
# Copyright (c) 2026 ALRIGROUP and its affiliates.
# UN1CA Carbon - Dynamic Boot Animation Controller

LOG_TAG="CarbonBootAnim"

log_msg() {
    log -t "$LOG_TAG" "$1"
}

STYLE="$(getprop persist.sys.unica.bootanim 2>/dev/null)"
[ -z "$STYLE" ] && STYLE="carbon"

if [ "$STYLE" = "rezoss" ]; then
    log_msg "Switching to Android 17 (REZOSS) boot animation"
    # Mask /system/media/bootanimation.zip with /dev/null so AOSP player plays built-in Android 17 logo
    mount -o bind /dev/null /system/media/bootanimation.zip 2>/dev/null || mount --bind /dev/null /system/media/bootanimation.zip 2>/dev/null || true
else
    log_msg "Switching to UN1CA Carbon boot animation (default)"
    # Ensure bootanimation.zip is unmasked so AOSP player plays UN1CA Carbon
    umount /system/media/bootanimation.zip 2>/dev/null || true
fi

exit 0
```

### 3.3. Init Fast-Path Triggers (`rezoss_bootanimation.rc`)
To eliminate shell latency and bypass SELinux boundaries completely, the `init.rc` configuration includes native kernel-level mount triggers:

```rc
service rezoss_bootanim /system/bin/bootanimation
    class core
    user graphics
    group graphics system
    disabled
    oneshot

on post-fs-data
    exec_background -- /system/bin/sh /system/etc/unica/rezoss_bootanimation.sh --apply

on property:persist.sys.unica.bootanim=*
    exec_background -- /system/bin/sh /system/etc/unica/rezoss_bootanimation.sh --apply

# Native init fast-path triggers
on post-fs-data && property:persist.sys.unica.bootanim=rezoss
    mount none /dev/null /system/media/bootanimation.zip bind

on post-fs-data && property:persist.sys.unica.bootanim=carbon
    umount /system/media/bootanimation.zip

on property:persist.sys.unica.bootanim=rezoss
    mount none /dev/null /system/media/bootanimation.zip bind

on property:persist.sys.unica.bootanim=carbon
    umount /system/media/bootanimation.zip
```

---

## 4. SELinux & Forensic Debugging

### 4.1. The `bootanim_exec` Domain Barrier
The executable `/system/bin/bootanimation` is labeled with SELinux context `u:object_r:bootanim_exec:s0`.

* **The Pitfall:** In legacy scripts, testing `[ ! -x "$AOSP_BOOTANIM" ]` inside a shell script resulted in *Permission denied* from SELinux. The shell script assumed the binary did not exist and aborted.
* **The Resolution:** All redundant `[ -x ]` or directory permission probes were excised from `rezoss_bootanimation.sh`. The file is guaranteed to exist at build time with proper permissions (`0755 root:graphics`).

### 4.2. Diagnostic Commands
To inspect and test the boot animation subsystem live via ADB:

```bash
# Check currently active style
adb shell getprop persist.sys.unica.bootanim

# Set style to Android 17 (REZOSS)
adb shell setprop persist.sys.unica.bootanim rezoss

# Set style to UN1CA Carbon (Default)
adb shell setprop persist.sys.unica.bootanim carbon

# Inspect SurfaceFlinger buffer allocation in logcat
adb shell "logcat -d | grep -i BootAnimation"
```
