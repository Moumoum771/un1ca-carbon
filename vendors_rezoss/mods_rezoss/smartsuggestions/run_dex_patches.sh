#!/usr/bin/env bash
set -eo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../../../.." && pwd)"

APK_PATH="$ROOT_DIR/out/target/dm1q/work_dir/system/system/priv-app/SamsungSmartSuggestions/SamsungSmartSuggestions.apk"
[ -f "$APK_PATH" ] || APK_PATH="$ROOT_DIR/out/target/dm1q/work_dir/system/priv-app/SamsungSmartSuggestions/SamsungSmartSuggestions.apk"

if [ ! -f "$APK_PATH" ]; then
    echo "SamsungSmartSuggestions.apk not found in work_dir, skipping"
    exit 0
fi

TMP_DIR="/tmp/carbon_smartsuggestions_dex_$$"
rm -rf "$TMP_DIR" && mkdir -p "$TMP_DIR"

if unzip -l "$APK_PATH" classes11.dex &>/dev/null; then
    unzip -q -p "$APK_PATH" classes11.dex > "$TMP_DIR/classes11.dex"
    unzip -q -p "$APK_PATH" classes13.dex > "$TMP_DIR/classes13.dex"
    unzip -q -p "$APK_PATH" classes14.dex > "$TMP_DIR/classes14.dex"
    unzip -q -p "$APK_PATH" classes15.dex > "$TMP_DIR/classes15.dex"
    unzip -q -p "$APK_PATH" AndroidManifest.xml > "$TMP_DIR/AndroidManifest.xml"

    python3 "$SCRIPT_DIR/patch_contents_visibility_dex.py" "$TMP_DIR/classes14.dex"
    python3 "$SCRIPT_DIR/patch_now_nudge_dex.py" "$TMP_DIR/classes15.dex"
    python3 "$SCRIPT_DIR/patch_smartsuggestions_dev_mode.py" "$TMP_DIR/classes11.dex" "$TMP_DIR/classes13.dex" "$TMP_DIR/classes15.dex" "$TMP_DIR/AndroidManifest.xml"

    cp -a "$APK_PATH" "$TMP_DIR/SamsungSmartSuggestions.apk"
    cd "$TMP_DIR" && zip -q -0 "SamsungSmartSuggestions.apk" classes11.dex classes13.dex classes14.dex classes15.dex AndroidManifest.xml

    CERT_PREFIX="aosp"
    [ -f "$ROOT_DIR/security/unica_platform.x509.pem" ] && CERT_PREFIX="unica"

    signapk "$ROOT_DIR/security/${CERT_PREFIX}_platform.x509.pem" "$ROOT_DIR/security/${CERT_PREFIX}_platform.pk8" \
            "$TMP_DIR/SamsungSmartSuggestions.apk" "$TMP_DIR/SamsungSmartSuggestions.signed.apk"

    mv -f "$TMP_DIR/SamsungSmartSuggestions.signed.apk" "$APK_PATH"
fi

rm -rf "$TMP_DIR"
exit 0
