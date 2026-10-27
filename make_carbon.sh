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
# =============================================================================

set -eo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TARGET_DEVICE=""

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
            if [ -d "$ROOT_DIR/core-unica/target_$arg" ] || [ -d "$ROOT_DIR/target_$arg" ]; then
                TARGET_DEVICE="$arg"
            else
                CLEAN_ARGS+=("$arg")
            fi
            ;;
    esac
done

# If no valid device argument was passed, default to the first available target folder
if [ -z "$TARGET_DEVICE" ]; then
    for t_dir in "$ROOT_DIR"/core-unica/target_* "$ROOT_DIR"/target_*; do
        if [ -d "$t_dir" ]; then
            TARGET_DEVICE="$(basename "$t_dir" | sed 's/target_//')"
            break
        fi
    done
fi

BUILDENV_PATH="$ROOT_DIR/core-unica/buildenv.sh"
if [ ! -f "$BUILDENV_PATH" ]; then
    echo "Error: buildenv.sh not found at $BUILDENV_PATH!" >&2
    exit 1
fi

# Prepare arguments to pass downstream into buildenv.sh
BUILDENV_ARGS=("${CLEAN_ARGS[@]}")
if [ "$DEBLOAT_TELEMETRY" = "false" ]; then
    BUILDENV_ARGS+=("--no-telemetry-debloat")
else
    BUILDENV_ARGS+=("--telemetry-debloat")
fi
if [ -n "$TARGET_DEVICE" ]; then
    BUILDENV_ARGS+=("$TARGET_DEVICE")
fi

echo "========================================================"
echo " UN1CA Carbon Build Orchestrator"
echo " Target Device       : $TARGET_DEVICE"
echo " Telemetry Debloat   : $DEBLOAT_TELEMETRY"
echo " Boot Animation      : $BOOTANIMATION_STYLE"
echo "========================================================"

# Source the UN1CA environment to load variables and the run_cmd function
source "$BUILDENV_PATH" "${BUILDENV_ARGS[@]}"

# Execute the build pipeline in the exact requested order
echo "==> [1/3] Running download_fw..."
run_cmd download_fw

echo "==> [2/3] Running extract_fw..."
run_cmd extract_fw

echo "==> [3/3] Running make_rom..."
run_cmd make_rom

echo "========================================================"
echo "  UN1CA Carbon build pipeline completed successfully!"
echo "========================================================"
