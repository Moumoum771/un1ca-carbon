#!/usr/bin/env bash
#
# Copyright (c) 2026 ALRIGROUP and its affiliates.
# Engineered and maintained by ALRI Development.
#
# This code is licensed under the ARGLP - ALRI GROUP LICENSE PERMISSIVE
# found in the LICENSE file in the root directory of this source tree
# and at: https://github.com/alrigroup/licenses
#
# =============================================================================
# UN1CA Carbon - Boot Animation Switcher
# Organization: ALRI Development (ALRIGROUP)
# Lead:         AlexsAR (@alexsanderalri)
# Systems:      AlexsanderALRI
# Allows developer to select at build time between:
# - "carbon" : Custom UN1CA Carbon animation (p1.mp4 24fps intro + breathing pulse loop)
# - "stock"  : Stock UN1CA / Samsung QMG animation
# Controlled by: BOOTANIMATION_STYLE (default: "carbon")
# =============================================================================

SKIPUNZIP=1

STYLE="${BOOTANIMATION_STYLE:-carbon}"
CARBON_ZIP="$MODPATH/bootanimation_carbon.zip"

if [ "$STYLE" == "carbon" ]; then
    if [ -f "$CARBON_ZIP" ]; then
        LOG "- Installing UN1CA Carbon custom boot animation (AOSP zip mode)"
        mkdir -p "$WORK_DIR/system/system/media"
        cp -a "$CARBON_ZIP" "$WORK_DIR/system/system/media/bootanimation.zip"
        SET_METADATA "system" "system/media/bootanimation.zip" 0 0 644 "u:object_r:system_file:s0"

        # Direct native replacement of Samsung stock QMG bootanimation with AOSP zip player
        AOSP_BIN="$SRC_DIR/vendors_rezoss/mods_rezoss/bootanimation_zip/system/bin/bootanimation_zip"
        if [ -f "$AOSP_BIN" ]; then
            LOG "- Installing AOSP zip bootanimation player as primary /system/bin/bootanimation"
            cp -a "$AOSP_BIN" "$WORK_DIR/system/system/bin/bootanimation"
            SET_METADATA "system" "system/bin/bootanimation" 0 2000 755 "u:object_r:bootanim_exec:s0"
        fi
    else
        LOGW "bootanimation_carbon.zip not found at $CARBON_ZIP, keeping stock"
    fi
else
    LOG "- Keeping stock UN1CA boot animation (QMG mode)"
    if [ -f "$WORK_DIR/system/system/media/bootanimation.zip" ]; then
        rm -f "$WORK_DIR/system/system/media/bootanimation.zip"
    fi
fi
