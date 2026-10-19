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
# CARBON ENGINE — NEXT-GENERATION BUILD ORCHESTRATOR ENTRYPOINT
# Project: UN1CA Carbon (Version: IX • One UI 9.0 • Android 17 • SDK 37)
# Organization: ALRI Development (ALRIGROUP)
# Systems:      AlexsAR (@alexsanderalri) & AlexsanderALRI
# =============================================================================

set -eo pipefail

ENGINE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$ENGINE_DIR/.." && pwd)"
BIN_PATH="$ENGINE_DIR/build/carbon-engine"

# 1. Ensure the native Carbon Engine binary is compiled
if [ ! -x "$BIN_PATH" ]; then
    echo "============================================================"
    echo "  [CARB-INIT] Building Native Carbon Engine (C++20)...      "
    echo "============================================================"
    mkdir -p "$ENGINE_DIR/build"
    cd "$ENGINE_DIR/build"
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make -j"$(nproc)" carbon-engine
    echo "✔ Carbon Engine built successfully!"
    echo ""
fi

# 2. Run the native orchestrator
cd "$PROJECT_ROOT"
exec "$BIN_PATH" build "$@"
