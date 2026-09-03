# PowerShell Automated / Scripted Installer for RecRoll (Windows 10+)
# Run as Administrator if installing to standard Program Files / Common Files directories

[CmdletBinding()]
param(
    [string]$SourceDir = "$PSScriptRoot\..\..\build\bin",
    [switch]$Force
)

$ErrorActionPreference = "Stop"

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host " RecRoll Audio Plugin - Windows Installer" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan

# Detect Architecture (ARM64 vs x64)
$arch = if ([System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture -eq [System.Runtime.InteropServices.Architecture]::Arm64) { "arm64" } else { "x64" }
Write-Host "[i] Detected Architecture: $arch" -ForegroundColor Yellow

$binDir = Join-Path $SourceDir $arch
if (-not (Test-Path $binDir)) {
    # Fallback to direct bin folder
    $binDir = $SourceDir
}

$vst3Target = "$env:CommonProgramFiles\VST3\RecRoll.vst3"
$clapTarget = "$env:CommonProgramFiles\CLAP"
$appTarget  = "$env:ProgramFiles\RecRoll"

# 1. Install VST3
$vst3Source = Join-Path $binDir "RecRoll.vst3"
if (Test-Path $vst3Source) {
    Write-Host "[+] Installing VST3 to $vst3Target..." -ForegroundColor Green
    if (Test-Path $vst3Target) { Remove-Item -Recurse -Force $vst3Target }
    Copy-Item -Recurse -Force $vst3Source $vst3Target
} else {
    Write-Warning "VST3 binary not found at $vst3Source"
}

# 2. Install CLAP
$clapSource = Join-Path $binDir "RecRoll.clap"
if (Test-Path $clapSource) {
    Write-Host "[+] Installing CLAP to $clapTarget\RecRoll.clap..." -ForegroundColor Green
    if (-not (Test-Path $clapTarget)) { New-Item -ItemType Directory -Force -Path $clapTarget | Out-Null }
    Copy-Item -Force $clapSource (Join-Path $clapTarget "RecRoll.clap")
} else {
    Write-Warning "CLAP binary not found at $clapSource"
}

# 3. Install Standalone Application
$appSource = Join-Path $binDir "RecRoll.exe"
if (Test-Path $appSource) {
    Write-Host "[+] Installing Standalone to $appTarget\RecRoll.exe..." -ForegroundColor Green
    if (-not (Test-Path $appTarget)) { New-Item -ItemType Directory -Force -Path $appTarget | Out-Null }
    Copy-Item -Force $appSource (Join-Path $appTarget "RecRoll.exe")

    # Start Menu Shortcut
    $startMenu = "$env:ProgramData\Microsoft\Windows\Start Menu\Programs\RecRoll.lnk"
    $wshell = New-Object -ComObject WScript.Shell
    $shortcut = $wshell.CreateShortcut($startMenu)
    $shortcut.TargetPath = Join-Path $appTarget "RecRoll.exe"
    $shortcut.Save()
} else {
    Write-Warning "Standalone binary not found at $appSource"
}

Write-Host "`n[✓] RecRoll installation completed successfully!" -ForegroundColor Cyan
