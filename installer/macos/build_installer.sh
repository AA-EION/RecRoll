#!/usr/bin/env bash
# macOS Universal Package & DMG Builder for RecRoll
# Generates signed PKG and DMG with ad-hoc signing for Gatekeeper compatibility.
# Compatible with macOS 11.0 Big Sur through macOS 26 Tahoe+

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
DIST_DIR="${ROOT_DIR}/dist"

VERSION="1.0.0"
IDENTIFIER="com.recrollaudio.recroll"

echo "=============================================="
echo " Building RecRoll macOS Universal PKG & DMG   "
echo "=============================================="

mkdir -p "${DIST_DIR}"
STAGE_DIR=$(mktemp -d /tmp/recroll_stage.XXXXXX)
trap 'rm -rf "${STAGE_DIR}"' EXIT

# Staging layout
VST3_STAGE="${STAGE_DIR}/vst3"
CLAP_STAGE="${STAGE_DIR}/clap"
AU_STAGE="${STAGE_DIR}/au"
APP_STAGE="${STAGE_DIR}/app"

mkdir -p "${VST3_STAGE}/Library/Audio/Plug-Ins/VST3"
mkdir -p "${CLAP_STAGE}/Library/Audio/Plug-Ins/CLAP"
mkdir -p "${AU_STAGE}/Library/Audio/Plug-Ins/Components"
mkdir -p "${APP_STAGE}/Applications"

# Locate build outputs
VST3_SRC="${BUILD_DIR}/RecRoll_artefacts/Release/VST3/RecRoll.vst3"
CLAP_SRC="${BUILD_DIR}/RecRoll_artefacts/Release/CLAP/RecRoll.clap"
AU_SRC="${BUILD_DIR}/RecRoll_artefacts/Release/AU/RecRoll.component"
APP_SRC="${BUILD_DIR}/RecRoll_artefacts/Release/Standalone/RecRoll.app"

# Fallbacks if artefacts are in standard CMake output
if [ ! -d "${VST3_SRC}" ]; then VST3_SRC=$(find "${BUILD_DIR}" -name "RecRoll.vst3" -type d | head -n 1 || true); fi
if [ ! -d "${CLAP_SRC}" ]; then CLAP_SRC=$(find "${BUILD_DIR}" -name "RecRoll.clap" -type d | head -n 1 || true); fi
if [ ! -d "${AU_SRC}" ]; then AU_SRC=$(find "${BUILD_DIR}" -name "RecRoll.component" -type d | head -n 1 || true); fi
if [ ! -d "${APP_SRC}" ]; then APP_SRC=$(find "${BUILD_DIR}" -name "RecRoll.app" -type d | head -n 1 || true); fi

# 1. Ad-Hoc Sign Binaries
echo "[*] Applying ad-hoc codesign to all bundles..."
if [ -d "${VST3_SRC}" ]; then
    cp -R "${VST3_SRC}" "${VST3_STAGE}/Library/Audio/Plug-Ins/VST3/"
    codesign --force --deep -s - "${VST3_STAGE}/Library/Audio/Plug-Ins/VST3/RecRoll.vst3"
fi

if [ -d "${CLAP_SRC}" ]; then
    cp -R "${CLAP_SRC}" "${CLAP_STAGE}/Library/Audio/Plug-Ins/CLAP/"
    codesign --force --deep -s - "${CLAP_STAGE}/Library/Audio/Plug-Ins/CLAP/RecRoll.clap"
fi

if [ -d "${AU_SRC}" ]; then
    cp -R "${AU_SRC}" "${AU_STAGE}/Library/Audio/Plug-Ins/Components/"
    codesign --force --deep -s - "${AU_STAGE}/Library/Audio/Plug-Ins/Components/RecRoll.component"
fi

if [ -d "${APP_SRC}" ]; then
    cp -R "${APP_SRC}" "${APP_STAGE}/Applications/"
    codesign --force --deep -s - "${APP_STAGE}/Applications/RecRoll.app"
fi

# 2. Build Component Packages
PKG_BUILD_TMP="${STAGE_DIR}/pkgs"
mkdir -p "${PKG_BUILD_TMP}"

echo "[*] Building individual component packages..."
pkgbuild --root "${VST3_STAGE}" --identifier "${IDENTIFIER}.vst3" --version "${VERSION}" --install-location "/" "${PKG_BUILD_TMP}/RecRoll-VST3.pkg"
pkgbuild --root "${CLAP_STAGE}" --identifier "${IDENTIFIER}.clap" --version "${VERSION}" --install-location "/" "${PKG_BUILD_TMP}/RecRoll-CLAP.pkg"
pkgbuild --root "${AU_STAGE}" --identifier "${IDENTIFIER}.au" --version "${VERSION}" --install-location "/" "${PKG_BUILD_TMP}/RecRoll-AU.pkg"
pkgbuild --root "${APP_STAGE}" --identifier "${IDENTIFIER}.app" --version "${VERSION}" --install-location "/" "${PKG_BUILD_TMP}/RecRoll-App.pkg"

# 3. Synthesize Product Distribution PKG
DIST_XML="${STAGE_DIR}/distribution.xml"
cat <<EOF > "${DIST_XML}"
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="1">
    <title>RecRoll ${VERSION}</title>
    <options customize="always" require-scripts="false" hostArchitectures="arm64,x86_64"/>
    <choices-outline>
        <line choice="choice_vst3"/>
        <line choice="choice_clap"/>
        <line choice="choice_au"/>
        <line choice="choice_app"/>
    </choices-outline>
    <choice id="choice_vst3" title="VST3 Plugin" description="Installs RecRoll VST3 plugin">
        <pkg-ref id="${IDENTIFIER}.vst3"/>
    </choice>
    <choice id="choice_clap" title="CLAP Plugin" description="Installs RecRoll CLAP plugin">
        <pkg-ref id="${IDENTIFIER}.clap"/>
    </choice>
    <choice id="choice_au" title="AudioUnit (AU) Plugin" description="Installs RecRoll AU component">
        <pkg-ref id="${IDENTIFIER}.au"/>
    </choice>
    <choice id="choice_app" title="Standalone Application" description="Installs RecRoll Standalone app">
        <pkg-ref id="${IDENTIFIER}.app"/>
    </choice>
    <pkg-ref id="${IDENTIFIER}.vst3" version="${VERSION}">RecRoll-VST3.pkg</pkg-ref>
    <pkg-ref id="${IDENTIFIER}.clap" version="${VERSION}">RecRoll-CLAP.pkg</pkg-ref>
    <pkg-ref id="${IDENTIFIER}.au" version="${VERSION}">RecRoll-AU.pkg</pkg-ref>
    <pkg-ref id="${IDENTIFIER}.app" version="${VERSION}">RecRoll-App.pkg</pkg-ref>
</installer-gui-script>
EOF

FINAL_PKG="${DIST_DIR}/RecRoll-macOS-Universal-Installer.pkg"
echo "[*] Generating final product package..."
productbuild --distribution "${DIST_XML}" --package-path "${PKG_BUILD_TMP}" "${FINAL_PKG}"

# 4. Create DMG containing PKG and Uninstaller script
echo "[*] Creating distributable DMG..."
DMG_STAGE="${STAGE_DIR}/dmg_stage"
mkdir -p "${DMG_STAGE}"
cp "${FINAL_PKG}" "${DMG_STAGE}/"
cp "${SCRIPT_DIR}/uninstall.sh" "${DMG_STAGE}/Uninstall RecRoll.command"
chmod +x "${DMG_STAGE}/Uninstall RecRoll.command"

# Copy readme
if [ -f "${ROOT_DIR}/README.md" ]; then
    cp "${ROOT_DIR}/README.md" "${DMG_STAGE}/"
fi

FINAL_DMG="${DIST_DIR}/RecRoll-macOS-Universal.dmg"
hdiutil create -volname "RecRoll ${VERSION}" -srcfolder "${DMG_STAGE}" -ov -format UDZO "${FINAL_DMG}"

# Ad-hoc sign the DMG
codesign --force -s - "${FINAL_DMG}" || true

echo "=============================================="
echo " [✓] macOS Build completed successfully!"
echo " Installer: ${FINAL_PKG}"
echo " Disk Image: ${FINAL_DMG}"
echo "=============================================="
