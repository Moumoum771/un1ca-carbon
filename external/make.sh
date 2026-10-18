#!/usr/bin/env bash
#
# Copyright (c) 2026 ALRIGROUP and its affiliates.
# Engineered and maintained by ALRI Development.
#
# This code is licensed under the ARGLP - ALRI GROUP LICENSE PERMISSIVE
# found in the LICENSE file in the root directory of this source tree
# and at: https://github.com/alrigroup/licenses
#

TOOLS_DIR="${TOOLS_DIR:-$SRC_DIR/out/tools}"

if [ "$1" == "--check-tools" ]; then
    if [ -f "$TOOLS_DIR/bin/apktool" ] && [ -f "$TOOLS_DIR/bin/mkfs.erofs" ] && [ -f "$TOOLS_DIR/bin/signapk" ]; then
        exit 0
    else
        exit 1
    fi
fi

exit 0
