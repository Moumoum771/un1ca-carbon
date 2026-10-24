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
# UN1CA Carbon Engine - Pure Native C++20 Build Orchestrator
# Project: UN1CA Carbon (Version: IX • One UI 9.0 • Android 17 • SDK 37)
# Device:  Samsung Galaxy S23 Series (dm1q / dm2q / dm3q)
# =============================================================================

set -eo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_PATH="$ROOT_DIR/carbon_engine/build/carbon-engine"
TARGET_DEVICE="dm1q"
SUBCOMMAND="build"
FORWARD_ARGS=()

for arg in "$@"; do
    case "$arg" in
        inspect|clean|verify|recipe|keys)
            SUBCOMMAND="$arg"
            ;;
        dm1q|dm2q|dm3q)
            TARGET_DEVICE="$arg"
            ;;
        *)
            FORWARD_ARGS+=("$arg")
            ;;
    esac
done

# 1. Ensure the native Carbon Engine binary is compiled and up to date
if [ ! -x "$BIN_PATH" ]; then
    echo "============================================================"
    echo "  [CARB-INIT] Building Native Carbon Engine (C++20)...      "
    echo "============================================================"
    mkdir -p "$ROOT_DIR/carbon_engine/build"
    cd "$ROOT_DIR/carbon_engine/build"
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make -j"$(nproc)" carbon-engine gen_manifest
    echo "✔ Carbon Engine built successfully!"
    echo ""
fi

# 2. Run the native orchestrator from the project root
cd "$ROOT_DIR"
mkdir -p "$ROOT_DIR/out"
LOG_FILE="$ROOT_DIR/out/build_engine.log"

if [ "$SUBCOMMAND" = "build" ]; then
    echo "• Logs will be streamed to console and saved in: $LOG_FILE"
    echo ""
    "$BIN_PATH" build -t "$TARGET_DEVICE" "${FORWARD_ARGS[@]}" 2>&1 | tee "$LOG_FILE"
else
    exec "$BIN_PATH" "$SUBCOMMAND" "${FORWARD_ARGS[@]}"
fi
