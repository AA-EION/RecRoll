# PowerShell Automated Uninstaller for RecRoll (Windows)
# Run as Administrator

[CmdletBinding()]
param()

$ErrorActionPreference = "Continue"

Write-Host "===========================================" -ForegroundColor Magenta
Write-Host " RecRoll Audio Plugin - Windows Uninstaller" -ForegroundColor Magenta
Write-Host "===========================================" -ForegroundColor Magenta

# 1. Stop any running RecRoll instances
Get-Process "RecRoll" -ErrorAction SilentlyContinue | Stop-Process -Force

# 2. Remove VST3
$vst3Target = "$env:CommonProgramFiles\VST3\RecRoll.vst3"
if (Test-Path $vst3Target) {
    Write-Host "[*] Removing VST3: $vst3Target" -ForegroundColor Yellow
    Remove-Item -Recurse -Force $vst3Target
}

# 3. Remove CLAP
$clapTarget = "$env:CommonProgramFiles\CLAP\RecRoll.clap"
if (Test-Path $clapTarget) {
    Write-Host "[*] Removing CLAP: $clapTarget" -ForegroundColor Yellow
    Remove-Item -Force $clapTarget
}

# 4. Remove Standalone Application & Shortcuts
$appTarget = "$env:ProgramFiles\RecRoll"
if (Test-Path $appTarget) {
    Write-Host "[*] Removing Standalone: $appTarget" -ForegroundColor Yellow
    Remove-Item -Recurse -Force $appTarget
}

$startMenuShortcut = "$env:ProgramData\Microsoft\Windows\Start Menu\Programs\RecRoll.lnk"
if (Test-Path $startMenuShortcut) {
    Remove-Item -Force $startMenuShortcut
}

$desktopShortcut = "$env:Public\Desktop\RecRoll.lnk"
if (Test-Path $desktopShortcut) {
    Remove-Item -Force $desktopShortcut
}

# 5. Clean Application Temp and Cache
$tempExports = Join-Path $env:TEMP "RecRoll_Exports"
if (Test-Path $tempExports) {
    Remove-Item -Recurse -Force $tempExports
}

Write-Host "`n[✓] RecRoll uninstalled successfully from this system." -ForegroundColor Green
