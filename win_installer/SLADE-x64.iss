#include "include/Defines.iss"

#include "include/Setup.iss"
ArchitecturesInstallIn64BitMode=x64
OutputBaseFilename=Setup_{#MyAppName}_{#MyAppVersion}_x64

#include "include/Languages.iss"

#include "include/Tasks.iss"

[Files]
Source: "..\dist\SLADE-x64.exe"; DestName: "SLADE.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\SLADE-x64.pdb"; DestName: "SLADE-x64.pdb"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\slade.pk3"; DestDir: "{app}"; Flags: ignoreversion

#include "include/Icons.iss"

#include "include/Run.iss"
