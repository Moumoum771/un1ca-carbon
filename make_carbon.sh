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
            if [ -d "$ROOT_DIR/carbon_ours/target_$arg" -o -d "$ROOT_DIR/target/$arg" ]; then
                TARGET_DEVICE="$arg"
            else
                CLEAN_ARGS+=("$arg")
            fi
            ;;
    esac
done

# If no valid device argument was passed, default to the first available target folder
if [ -z "$TARGET_DEVICE" ]; then
    for t_dir in "$ROOT_DIR"/carbon_ours/target_*; do
        if [ -d "$t_dir" ]; then
            TARGET_DEVICE="$(basename "$t_dir" | sed 's/target_//')"
            break
        fi
    done
fi