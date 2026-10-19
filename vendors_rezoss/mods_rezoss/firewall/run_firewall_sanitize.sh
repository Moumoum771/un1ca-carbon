#!/usr/bin/env bash
set -eo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../../../.." && pwd)"

FIREWALL_APK="$ROOT_DIR/out/target/dm1q/apktool/system/priv-app/Firewall/Firewall.apk"
TSV_FILE="$SCRIPT_DIR/region_names.tsv"

if [ -d "$FIREWALL_APK" ] && [ -f "$TSV_FILE" ]; then
    python3 "$SCRIPT_DIR/patch_region_strings.py" "$FIREWALL_APK" "$TSV_FILE"
fi

exit 0
