; Inno Setup 6 Script for RecRoll Universal Windows Installer (x64 + arm64)
; Target: Windows 10 and Windows 11 (Intel/AMD x64 and ARM64 Snapdragon)

#define MyAppName "RecRoll"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "EION Studios"
#define MyAppURL "https://eionstudios.com"
#define MyAppSupportURL "https://github.com/AA-EION/RecRoll"
#define MyAppExeName "RecRoll.exe"

[Setup]
AppId={{E1CE670E-2A2F-5F0B-B948-D70441B507AA}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppSupportURL}
AppUpdatesURL={#MyAppSupportURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
LicenseFile=..\..\LICENSE
OutputDir=..\..\dist
OutputBaseFilename=RecRoll-Windows-Universal-Installer
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
SetupIconFile=..\..\Resources\RecRoll_Icon.ico
ArchitecturesInstallIn64BitMode=x64 arm64
MinVersion=10.0
UninstallDisplayIcon={app}\{#MyAppExeName}
UninstallDisplayName={#MyAppName} {#MyAppVersion}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Types]
Name: "full"; Description: "Full installation (VST3, CLAP, and Standalone)"
Name: "compact"; Description: "Plugins only (VST3 and CLAP)"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 Plugin (.vst3)"; Types: full compact custom; Flags: checkablealone
Name: "clap"; Description: "CLAP Plugin (.clap)"; Types: full compact custom; Flags: checkablealone
Name: "standalone"; Description: "Standalone Application"; Types: full custom; Flags: checkablealone

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Components: standalone; Flags: unchecked

[Files]
; --- x64 Architecture Binaries ---
; VST3
Source: "..\..\build\bin\x64\RecRoll.vst3\*"; DestDir: "{commoncf}\VST3\RecRoll.vst3"; Check: IsX64 and not IsArm64; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
; CLAP
Source: "..\..\build\bin\x64\RecRoll.clap"; DestDir: "{commoncf}\CLAP"; Check: IsX64 and not IsArm64; Components: clap; Flags: ignoreversion skipifsourcedoesntexist
; Standalone
Source: "..\..\build\bin\x64\RecRoll.exe"; DestDir: "{app}"; Check: IsX64 and not IsArm64; Components: standalone; Flags: ignoreversion

; --- ARM64 Architecture Binaries (Snapdragon X / Windows on ARM) ---
; VST3
Source: "..\..\build\bin\arm64\RecRoll.vst3\*"; DestDir: "{commoncf}\VST3\RecRoll.vst3"; Check: IsArm64; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
; CLAP
Source: "..\..\build\bin\arm64\RecRoll.clap"; DestDir: "{commoncf}\CLAP"; Check: IsArm64; Components: clap; Flags: ignoreversion skipifsourcedoesntexist
; Standalone
Source: "..\..\build\bin\arm64\RecRoll.exe"; DestDir: "{app}"; Check: IsArm64; Components: standalone; Flags: ignoreversion

; Common Documentation
Source: "..\..\README.md"; DestDir: "{app}"; Flags: ignoreversion; Components: standalone
Source: "..\..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion; Components: standalone

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Components: standalone
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"; Components: standalone
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon; Components: standalone

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent; Components: standalone

[Code]
// Helper function to detect ARM64 Windows host
function IsArm64: Boolean;
begin
  Result := (ProcessorArchitecture = paARM64);
end;

function IsX64: Boolean;
begin
  Result := (ProcessorArchitecture = paX64);
end;
