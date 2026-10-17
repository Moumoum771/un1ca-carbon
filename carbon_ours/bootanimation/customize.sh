#!/usr/bin/env bash
# =============================================================================
# UN1CA Carbon - Boot Animation Switcher
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
    else
        LOGW "bootanimation_carbon.zip not found at $CARBON_ZIP, keeping stock"
    fi
else
    LOG "- Keeping stock UN1CA boot animation (QMG mode)"
    if [ -f "$WORK_DIR/system/system/media/bootanimation.zip" ]; then
        rm -f "$WORK_DIR/system/system/media/bootanimation.zip"
    fi
fi
