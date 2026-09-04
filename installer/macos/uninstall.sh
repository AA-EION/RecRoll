#!/usr/bin/env bash
# macOS Complete Uninstaller for RecRoll
# Purges Standalone, VST3, CLAP, AU, Application Support, Preferences, Caches, LaunchAgents, and Receipts.

set -euo pipefail

BUNDLE_ID="com.eionstudios.recroll"

echo "=============================================="
echo " RecRoll Complete Uninstaller for macOS       "
echo "=============================================="

# Elevate to sudo if running as standard user
if [ "$(id -u)" -ne 0 ]; then
    echo "[!] Requesting administrator privileges to remove system audio plugins..."
    exec sudo bash "$0" "$@"
fi

# 1. Terminate any running instances
killall "RecRoll" 2>/dev/null || true

echo "[*] Removing application and plugin bundles..."
rm -rf "/Applications/RecRoll.app"
rm -rf "/Library/Audio/Plug-Ins/VST3/RecRoll.vst3"
rm -rf "/Library/Audio/Plug-Ins/CLAP/RecRoll.clap"
rm -rf "/Library/Audio/Plug-Ins/Components/RecRoll.component"

# User-specific plugin folders (for all non-system users)
for USER_DIR in /Users/*; do
    if [ -d "${USER_DIR}" ] && [ "${USER_DIR}" != "/Users/Shared" ]; then
        rm -rf "${USER_DIR}/Library/Audio/Plug-Ins/VST3/RecRoll.vst3"
        rm -rf "${USER_DIR}/Library/Audio/Plug-Ins/CLAP/RecRoll.clap"
        rm -rf "${USER_DIR}/Library/Audio/Plug-Ins/Components/RecRoll.component"
        rm -rf "${USER_DIR}/Library/Application Support/RecRoll"
        rm -rf "${USER_DIR}/Library/Preferences/${BUNDLE_ID}".*
        rm -rf "${USER_DIR}/Library/Caches/${BUNDLE_ID}".*
        rm -rf "${USER_DIR}/Library/Saved Application State/${BUNDLE_ID}".*
        rm -rf "${USER_DIR}/Library/LaunchAgents/${BUNDLE_ID}".*
    fi
done

echo "[*] Removing system support files, caches, and launch agents..."
rm -rf "/Library/Application Support/RecRoll"
rm -rf "/Library/LaunchAgents/${BUNDLE_ID}".*
rm -rf "/Library/LaunchDaemons/${BUNDLE_ID}".*

echo "[*] Forgetting package receipts..."
for SUFFIX in "" ".vst3" ".clap" ".au" ".app"; do
    pkgutil --forget "${BUNDLE_ID}${SUFFIX}" 2>/dev/null || true
done
rm -f /var/db/receipts/"${BUNDLE_ID}".* 2>/dev/null || true

echo "[*] Refreshing macOS AudioComponentRegistrar cache..."
killall -9 AudioComponentRegistrar 2>/dev/null || true

echo "=============================================="
echo " [✓] RecRoll uninstallation complete!"
echo "=============================================="
