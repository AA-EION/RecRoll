#!/usr/bin/env bash
# macOS Complete Uninstaller for RecRoll
# Purges Standalone, VST3, CLAP, AU, Application Support, Preferences, Caches, LaunchAgents, and Receipts.

set -euo pipefail

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
        rm -rf "${USER_DIR}/Library/Preferences/com.recrollaudio.*"
        rm -rf "${USER_DIR}/Library/Caches/com.recrollaudio.*"
        rm -rf "${USER_DIR}/Library/Saved Application State/com.recrollaudio.*"
        rm -rf "${USER_DIR}/Library/LaunchAgents/com.recrollaudio.*"
    fi
done

echo "[*] Removing system support files, caches, and launch agents..."
rm -rf "/Library/Application Support/RecRoll"
rm -rf "/Library/LaunchAgents/com.recrollaudio.*"
rm -rf "/Library/LaunchDaemons/com.recrollaudio.*"

echo "[*] Forgetting package receipts..."
pkgutil --forget com.recrollaudio.recroll 2>/dev/null || true
pkgutil --forget com.recrollaudio.recroll.vst3 2>/dev/null || true
pkgutil --forget com.recrollaudio.recroll.clap 2>/dev/null || true
pkgutil --forget com.recrollaudio.recroll.au 2>/dev/null || true
pkgutil --forget com.recrollaudio.recroll.app 2>/dev/null || true
rm -f /var/db/receipts/com.recrollaudio.* 2>/dev/null || true

echo "[*] Refreshing macOS AudioComponentRegistrar cache..."
killall -9 AudioComponentRegistrar 2>/dev/null || true

echo "=============================================="
echo " [✓] RecRoll uninstallation complete!"
echo "=============================================="
