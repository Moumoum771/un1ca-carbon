#!/usr/bin/env bash
# =============================================================================
# UN1CA Carbon Build Orchestrator
# Project: UN1CA Carbon (Version: IX • One UI 9.0 • Android 17 • SDK 37)
# Device:  Samsung Galaxy S23 Series (dm1q / dm2q / dm3q)
# =============================================================================

set -eo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TARGET_DEVICE="dm1q"

# Configurable Telemetry Debloater (default: true)
# Can be toggled via environment variable: DEBLOAT_TELEMETRY=false ./make_carbon.sh
# Or via CLI arguments: ./make_carbon.sh --no-telemetry-debloat
export DEBLOAT_TELEMETRY="${DEBLOAT_TELEMETRY:-true}"

CLEAN_ARGS=()
for arg in "$@"; do
    case "$arg" in
        --no-telemetry-debloat|--disable-telemetry-debloat|--no-debloat)
            export DEBLOAT_TELEMETRY=false
            ;;
        --telemetry-debloat|--enable-telemetry-debloat)
            export DEBLOAT_TELEMETRY=true
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
echo "      UN1CA CARBON — ROM BUILD PIPELINE                     "
echo "  Version: IX (One UI 9.0 • Android 17)                     "
echo "  Target: $TARGET_DEVICE                                    "
echo "  Lead Maintainer: AlexsAR (@alexsanderalri)                "
echo "  Port & Engineering: AlexsanderALRI                        "
echo "  Debloat Telemetry: $DEBLOAT_TELEMETRY                     "
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

# Config links
for cfg in "$ROOT_DIR"/carbon_ours/configs/*.sh; do
    [ -f "$cfg" ] || continue
    cname="$(basename "$cfg")"
    [ ! -e "$ROOT_DIR/unica/configs/$cname" ] && ln -s "$cfg" "$ROOT_DIR/unica/configs/$cname"
done

# 2. Target links
[ ! -e "$ROOT_DIR/target/dm1q" ] && ln -s "$ROOT_DIR/carbon_ours/target_dm1q" "$ROOT_DIR/target/dm1q"

# 3. Vendors modules links
[ ! -e "$ROOT_DIR/unica/mods/rezoss" ] && ln -s "$ROOT_DIR/vendors_rezoss/mods_rezoss" "$ROOT_DIR/unica/mods/rezoss"
[ ! -e "$ROOT_DIR/unica/mods/hma" ] && ln -s "$ROOT_DIR/vendors_rezoss/mods_hma" "$ROOT_DIR/unica/mods/hma"
[ ! -e "$ROOT_DIR/unica/patches/signature" ] && ln -s "$ROOT_DIR/vendors_rezoss/patches_signature" "$ROOT_DIR/unica/patches/signature"

[ ! -e "$ROOT_DIR/unica/mods/cosine" ] && ln -s "$ROOT_DIR/vendors_npl/mods_cosine" "$ROOT_DIR/unica/mods/cosine"

# 4. Carbon ours modules links
[ ! -e "$ROOT_DIR/unica/mods/settings" ] && ln -s "$ROOT_DIR/carbon_ours/branding_settings" "$ROOT_DIR/unica/mods/settings"
[ ! -e "$ROOT_DIR/unica/mods/telemetry_debloat" ] && ln -s "$ROOT_DIR/carbon_ours/telemetry_debloat" "$ROOT_DIR/unica/mods/telemetry_debloat"

echo "✔ Architecture tree verified and linked."
echo "Starting ROM build for $TARGET_DEVICE..."
echo ""

cd "$ROOT_DIR"
source "$ROOT_DIR/core_unica/buildenv.sh" "$TARGET_DEVICE"
exec "$ROOT_DIR/scripts/make_rom.sh" -z "${CLEAN_ARGS[@]}"
