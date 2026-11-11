#define MyAppName "SLADE"
#define MyAppVersion "3.3.1"
#define MyAppURL "https://slade.mancubus.net"
#define MyAppExeName "SLADE.exe"
#define MyAppPublisher "sirjuddington"

[Setup]
AppId={{3EFD0AA9-5156-40DB-9646-360180FF5DFA}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} v{#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={commonpf}\{#MyAppName}
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
SetupIconFile=..\msvc\slade.ico
Compression=lzma
SolidCompression=yes
WizardStyle=modern dynamic
ArchitecturesInstallIn64BitMode=x64compatible
OutputBaseFilename={#MyAppName}_{#MyAppVersion}_win_x64_setup

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "sendtoicon"; Description: "Create 'Send To' shortcut"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "..\dist\build64\SLADE.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\build64\SLADE.pdb"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\slade.pk3"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\Tools\*"; DestDir: "{app}\Tools"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{commondesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon
Name: "{usersendto}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: sendtoicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
