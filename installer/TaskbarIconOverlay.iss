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

[CustomMessages]
CreateStartMenuIcon=Create a &Start Menu shortcut

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "startmenuicon"; Description: "{cm:CreateStartMenuIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
Source: "..\artifacts\publish\TaskbarIconOverlay-{#MyAppVersion}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autostartup}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: startmenuicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch {#MyAppName}"; Flags: nowait postinstall skipifsilent
