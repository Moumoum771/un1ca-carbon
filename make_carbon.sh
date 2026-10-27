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
# UN1CA Carbon Build Orchestrator
# Project: UN1CA Carbon (Version: IX • One UI 9.0 • Android 17 • SDK 37)
# Device:  Samsung Galaxy S23 Series (dm1q / dm2q / dm3q)
# =============================================================================

set -eo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TARGET_DEVICE="dm1q"

# Configurable Telemetry Debloater (default: true)
export DEBLOAT_TELEMETRY="${DEBLOAT_TELEMETRY:-true}"

# Configurable Boot Animation (default: "carbon")
export BOOTANIMATION_STYLE="${BOOTANIMATION_STYLE:-carbon}"

CLEAN_ARGS=()
for arg in "$@"; do
    case "$arg" in
        --no-telemetry-debloat|--disable-telemetry-debloat|--no-debloat)
            export DEBLOAT_TELEMETRY=false
            ;;
        --telemetry-debloat|--enable-telemetry-debloat)
            export DEBLOAT_TELEMETRY=true
            ;;
        --bootanim=stock|--bootanim-stock)
            export BOOTANIMATION_STYLE="stock"
            ;;
        --bootanim=carbon|--bootanim-carbon)
            export BOOTANIMATION_STYLE="carbon"
            ;;
        -*)
            CLEAN_ARGS+=("$arg")
            ;;
        *)
            if [ "$TARGET_DEVICE" = "dm1q" ] && [ -d "$ROOT_DIR/carbon_ours/target_$arg" -o -d "$ROOT_DIR/target/$arg" ]; then
                TARGET_DEVICE="$arg"
            else
                CLEAN_ARGS+=("$arg")
            fi
            ;;
    esac
done

echo "============================================================"
echo "    UN1CA CARBON — ROM BUILD PIPELINE                     "
echo "  Version: IX (One UI 9.0 • Android 17)                     "
echo "  Target: $TARGET_DEVICE                                    "
echo "  Organization: ALRI Development (ALRIGROUP)                "
echo "  Lead Developer: Alexsander \"AlexsAR\" ALRI (@alexsanderalri)"
echo "  Port & Systems Engineering: ALRI Development              "
echo "============================================================"

# Ensure symlinked execution tree is present
mkdir -p "$ROOT_DIR/target"
mkdir -p "$ROOT_DIR/unica/mods"
mkdir -p "$ROOT_DIR/unica/patches"
mkdir -p "$ROOT_DIR/unica/configs"

# 1. Core UN1CA links
[ ! -e "$ROOT_DIR/scripts" ] && ln -s "$ROOT_DIR/core_unica/scripts" "$ROOT_DIR/scripts"
[ ! -e "$ROOT_DIR/tools" ] && ln -s "$ROOT_DIR/core_unica/tools" "$ROOT_DIR/tools"
[ ! -e "$ROOT_DIR/platform" ] && ln -s "$ROOT_DIR/core_unica/platform" "$ROOT_DIR/platform"
[ ! -e "$ROOT_DIR/security" ] && ln -s "$ROOT_DIR/carbon_ours/security_keys" "$ROOT_DIR/security"
[ ! -e "$ROOT_DIR/prebuilts" ] && ln -s "$ROOT_DIR/core_unica/prebuilts" "$ROOT_DIR/prebuilts"

# Link all core patches
for p in "$ROOT_DIR"/core_unica/patches/*; do
    [ -d "$p" ] || continue
    pname="$(basename "$p")"
    [ ! -e "$ROOT_DIR/unica/patches/$pname" ] && ln -s "$p" "$ROOT_DIR/unica/patches/$pname"
done

# Link all core mods
for m in "$ROOT_DIR"/core_unica/mods/*; do
    [ -d "$m" ] || continue
    mname="$(basename "$m")"
    [ ! -e "$ROOT_DIR/unica/mods/$mname" ] && ln -s "$m" "$ROOT_DIR/unica/mods/$mname"
done

# Config links
for cfg in "$ROOT_DIR"/carbon_ours/configs/*.sh; do
    [ -f "$cfg" ] || continue
    cname="$(basename "$cfg")"
    rm -f "$ROOT_DIR/unica/configs/$cname"
    ln -s "$cfg" "$ROOT_DIR/unica/configs/$cname"
done

# 2. Target links
rm -f "$ROOT_DIR/target/dm1q"
ln -s "$ROOT_DIR/carbon_ours/target_dm1q" "$ROOT_DIR/target/dm1q"

# 3. Vendors modules links
rm -f "$ROOT_DIR/unica/mods/rezoss" "$ROOT_DIR/unica/mods/hma" "$ROOT_DIR/unica/patches/signature" "$ROOT_DIR/unica/mods/cosine"
ln -s "$ROOT_DIR/vendors_rezoss/mods_rezoss" "$ROOT_DIR/unica/mods/rezoss"
ln -s "$ROOT_DIR/vendors_rezoss/mods_hma" "$ROOT_DIR/unica/mods/hma"
ln -s "$ROOT_DIR/vendors_rezOSS/patches_signature" "$ROOT_DIR/unica/patches/signature" 2>/dev/null || true
ln -s "$ROOT_DIR/vendors_rezoss/patches_signature" "$ROOT_DIR/unica/patches/signature" 2>/dev/null || true
ln -s "$ROOT_DIR/vendors_npl/mods_cosine" "$ROOT_DIR/unica/mods/cosine"

# 4. Carbon ours modules links
rm -f "$ROOT_DIR/unica/mods/settings" "$ROOT_DIR/unica/mods/telemetry_debloat" "$ROOT_DIR/unica/mods/carbon_bootanim" "$ROOT_DIR/unica/mods/kernel_engine"
ln -s "$ROOT_DIR/carbon_ours/branding_settings" "$ROOT_DIR/unica/mods/settings"
ln -s "$ROOT_DIR/carbon_ours/telemetry_debloat" "$ROOT_DIR/unica/mods/telemetry_debloat"
ln -s "$ROOT_DIR/carbon_ours/bootanimation" "$ROOT_DIR/unica/mods/carbon_bootanim"
ln -s "$ROOT_DIR/carbon_ours/kernel_engine" "$ROOT_DIR/unica/mods/kernel_engine"

echo "✔ Architecture tree verified and linked."
echo "Starting ROM build pipeline for $TARGET_DEVICE..."
echo ""

cd "$ROOT_DIR"
source "$ROOT_DIR/core_unica/buildenv.sh" "$TARGET_DEVICE"

# Sequential Execution Pipeline
echo "==> [1/3] Downloading firmware..."
"$ROOT_DIR/scripts/download_fw.sh" "${CLEAN_ARGS[@]}"

echo "==> [2/3] Extracting firmware..."
"$ROOT_DIR/scripts/extract_fw.sh" "${CLEAN_ARGS[@]}"

echo "==> [3/3] Building ROM..."
exec "$ROOT_DIR/scripts/make_rom.sh" -r -f "${CLEAN_ARGS[@]}"
