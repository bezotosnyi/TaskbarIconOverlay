; Build with: ISCC.exe /DMyAppVersion=0.1.0 installer\TaskbarIconOverlay.iss
#ifndef MyAppVersion
  #error MyAppVersion must be supplied, e.g. /DMyAppVersion=0.1.0
#endif

#define MyAppName "TaskbarIconOverlay"
#define MyAppExeName "TaskbarIconOverlay.App.exe"

[Setup]
AppId={{8B3F9A09-2F8E-42F8-A896-C9F793B5E19D}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher=Dmytro Bezotosnyi
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
OutputDir=..\artifacts\publish
OutputBaseFilename=TaskbarIconOverlay-Setup-x64
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
UsePreviousAppDir=no
CloseApplications=yes
RestartApplications=no
UninstallDisplayIcon={app}\{#MyAppExeName}
SetupIconFile=..\src\app\Assets\AppIcon.ico

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "..\artifacts\publish\TaskbarIconOverlay-{#MyAppVersion}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autostartup}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch {#MyAppName}"; Flags: nowait postinstall skipifsilent
