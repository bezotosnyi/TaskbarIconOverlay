; Build with: ISCC.exe /DMyAppVersion=0.1.0 installer\TaskbarIconOverlay.iss
#ifndef MyAppVersion
  #error MyAppVersion must be supplied, e.g. /DMyAppVersion=0.1.0
#endif

#define MyAppName "TaskbarIconOverlay"
#define MyAppExeName "TaskbarIconOverlay.App.exe"
#define MyAppID "DmytroBezotosnyi.TaskbarIconOverlay"

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
ChangesAssociations=yes
CloseApplications=yes
RestartApplications=no
UninstallDisplayIcon={app}\{#MyAppExeName}
SetupIconFile=..\src\app\Assets\AppIcon.ico
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "ukrainian"; MessagesFile: "compiler:Languages\Ukrainian.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[CustomMessages]
; --- Start Menu Icon Custom Translations ---
english.CreateStartMenuIcon=Create a &Start Menu shortcut
ukrainian.CreateStartMenuIcon=Створити ярлик у меню &Пуск
russian.CreateStartMenuIcon=Создать ярлык в &меню Пуск

; --- Startup Group Headers Custom Translations ---
english.AdditionalOptions=Additional options:
ukrainian.AdditionalOptions=Додаткові опції:
russian.AdditionalOptions=Дополнительные параметры:

; --- Startup Task Custom Translations ---
english.RunAtStartup=Launch {#MyAppName} when Windows starts
ukrainian.RunAtStartup=Запускати {#MyAppName} при старті Windows
russian.RunAtStartup=Запускать {#MyAppName} при старте Windows

[Tasks]
; Standard Built-in Icons
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "startmenuicon"; Description: "{cm:CreateStartMenuIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

; Custom Startup Options
Name: "runatstartup"; Description: "{cm:RunAtStartup}"; GroupDescription: "{cm:AdditionalOptions}"; Flags: unchecked


[Files]
Source: "..\artifacts\publish\TaskbarIconOverlay-{#MyAppVersion}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autostartup}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: runatstartup
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: startmenuicon; AppUserModelID: "{#MyAppID}"

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch {#MyAppName}"; Flags: nowait postinstall skipifsilent
