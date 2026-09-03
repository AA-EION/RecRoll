# RecRoll - Open-Source Rolling Sampler & Retrospective Recorder

[![Build and Release RecRoll](https://github.com/recrollaudio/recroll/actions/workflows/build-and-release.yml/badge.svg)](https://github.com/recrollaudio/recroll/actions)
[![License: GPL-3.0](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![Platforms: Windows & macOS](https://img.shields.io/badge/Platforms-Windows%20%7C%20macOS-brightgreen.svg)]()
[![Formats: VST3, CLAP, AU, Standalone](https://img.shields.io/badge/Formats-VST3%20%7C%20CLAP%20%7C%20AU%20%7C%20Standalone-orange.svg)]()

**RecRoll** is a high-performance, open-source retrospective audio recording plugin and standalone application inspired by *Bird's Rolling Sampler*. 

It constantly records the audio passing through your DAW channel into a circular RAM buffer **without coloring or altering the incoming signal in any way** (100% bit-exact passthrough, zero latency). Whenever inspiration strikes or a happy accident occurs, simply highlight the region and **drag-and-drop it straight into your DAW timeline or desktop** as a pristine 24-bit WAV file.

---

## Key Features

- **Transparent Passthrough**: Bit-exact audio forwarding with **0 latency samples** added to your host.
- **Lock-Free Circular Audio Buffer**: Realtime-safe DSP engine with zero memory allocations or locks on the audio thread. Supports up to 10 minutes of continuous stereo audio history.
- **Buffer Presets**: Instant switching between `15s`, `30s`, `1m`, `2m`, `5m`, and `10m` view ranges.
- **Sleek Waveform Visualization**: 60 FPS multi-scale peak overview cache with time rulers, zero-crossing guide, and playhead indicators.
- **In-Plugin Auditioning**: Preview highlighted slices directly inside the plugin before dragging (press Spacebar or click Audition).
- **Direct DAW Drag-and-Drop**: Native OS drag-and-drop integration. Drag selections directly into Ableton Live, FL Studio, Reaper, Logic Pro, Cubase, Studio One, Bitwig, or your file explorer.
- **Normalize Option**: Optional one-click peak normalization (-0.1 dBFS) on export.
- **Freeze & Clear**: Pause buffer updates to inspect or cut audio without losing current samples, or flush history instantly.
- **Universal Binaries**:
  - **Windows**: Multi-architecture (`x64` Intel/AMD + `ARM64` Snapdragon X / Windows on ARM), targeting Windows 10+.
  - **macOS**: Universal 2 (`arm64` Apple Silicon + `x86_64` Intel), targeting macOS 11.0 Big Sur through macOS 26 Tahoe+.
- **Ad-Hoc Code Signed**: Ad-hoc code signed out of the box (`codesign -s -`) for smooth macOS Gatekeeper compatibility.
- **Full GUI Uninstaller**: Standalone app includes a built-in GUI self-uninstaller that cleans all plugins, preferences, cache, launch agents, and install receipts with admin confirmation.

---

## Supported Formats

| Format | Windows (x64 / ARM64) | macOS (Universal 2: Apple Silicon + Intel) |
|---|---|---|
| **VST3** | `Common Files\VST3\RecRoll.vst3` | `/Library/Audio/Plug-Ins/VST3/RecRoll.vst3` |
| **CLAP** | `Common Files\CLAP\RecRoll.clap` | `/Library/Audio/Plug-Ins/CLAP/RecRoll.clap` |
| **AU (AudioUnit v2)** | — | `/Library/Audio/Plug-Ins/Components/RecRoll.component` |
| **Standalone** | `Program Files\RecRoll\RecRoll.exe` | `/Applications/RecRoll.app` |

---

## Download & Installation

Pre-built binaries and installers are automatically built by GitHub Actions on every release:

👉 **[Download the Latest Release](../../releases/latest)**

### Windows (10+)
1. Download **`RecRoll-Windows-Universal-Installer.exe`**.
2. Run the installer. It automatically detects whether your system is **x64** or **ARM64** and installs the native plugins and standalone app.
3. *Alternative (Portable)*: Download `RecRoll-Windows-x64-Portable.zip` or `RecRoll-Windows-arm64-Portable.zip`.

### macOS (11.0 to macOS 26 Tahoe+)
1. Download **`RecRoll-macOS-Universal.dmg`** or **`RecRoll-macOS-Universal-Installer.pkg`**.
2. Run the installer package to install all plugin formats and the Standalone app.
3. **Gatekeeper Note**: Because RecRoll is open-source and ad-hoc signed, if macOS displays an *"unidentified developer"* notice upon first launch:
   - Right-click (Control-click) `RecRoll.app` in `/Applications` and select **Open**.
   - Or open **System Settings > Privacy & Security** and click **Open Anyway**.
   - Or run in Terminal:
     ```bash
     xattr -cr /Applications/RecRoll.app
     xattr -cr /Library/Audio/Plug-Ins/VST3/RecRoll.vst3
     xattr -cr /Library/Audio/Plug-Ins/CLAP/RecRoll.clap
     xattr -cr /Library/Audio/Plug-Ins/Components/RecRoll.component
     ```

---

## Uninstallation

### macOS (Full GUI Self-Uninstaller)
RecRoll provides a complete GUI self-uninstaller:
1. Open the **RecRoll Standalone** application from `/Applications/RecRoll.app`.
2. Click the **"Uninstall..."** button in the bottom right corner.
3. Confirm the prompt and enter your administrator password or Touch ID.
4. RecRoll will automatically delete:
   - `/Applications/RecRoll.app`
   - `/Library/Audio/Plug-Ins/VST3/RecRoll.vst3` & user VST3
   - `/Library/Audio/Plug-Ins/CLAP/RecRoll.clap` & user CLAP
   - `/Library/Audio/Plug-Ins/Components/RecRoll.component` & user AU
   - Application Support, Caches, Preferences, and Saved State
   - System package receipts (`pkgutil --forget com.recrollaudio.recroll`)
   - Resets the `AudioComponentRegistrar` cache.
5. *Command Line Alternative*: Run `./installer/macos/uninstall.sh` with `sudo`.

### Windows
- Open **Windows Settings > Apps > Installed apps**, find **RecRoll**, and click **Uninstall**.
- Or run `installer/windows/uninstall.ps1` in PowerShell as Administrator.

---

## CI/CD Pipeline (GitHub Actions)

This repository includes a multi-platform CI/CD workflow located at [`.github/workflows/build-and-release.yml`](.github/workflows/build-and-release.yml).

### How to Build & Release Automatically:
1. Push this repository to your GitHub account:
   ```bash
   git init
   git add .
   git commit -m "Initial commit of RecRoll"
   git remote add origin https://github.com/<your-username>/RecRoll.git
   git push -u origin main
   ```
2. To create an automated GitHub release with all Windows & macOS installers attached:
   ```bash
   git tag v1.0.0
   git push origin v1.0.0
   ```
3. Or manually trigger the build at any time from the **Actions** tab by selecting **"Build and Release RecRoll"** and clicking **Run workflow**.

---

## Building from Source

### Requirements
- **CMake** 3.22 or newer
- **C++20** compatible compiler:
  - Windows: Visual Studio 2022 (MSVC)
  - macOS: Xcode 14+ / Apple Clang
- Git (with submodule support)

JUCE 8 and `clap-juce-extensions` are automatically fetched via CMake `FetchContent`.

### Build on Windows (x64 or ARM64)
```powershell
# Configure for x64
cmake -B build-x64 -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build-x64 --config Release --parallel

# Or configure for Windows on ARM (ARM64)
cmake -B build-arm64 -A ARM64 -DCMAKE_BUILD_TYPE=Release
cmake --build build-arm64 --config Release --parallel
```

### Build on macOS (Universal 2: ARM64 + x86_64)
```bash
# Configure Universal binary
cmake -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="11.0"

# Build all formats
cmake --build build --config Release --parallel

# Package PKG and DMG
./installer/macos/build_installer.sh
```

---

## License

This project is licensed under the **GNU General Public License v3.0 (GPLv3)**. See the [LICENSE](LICENSE) file for details.
