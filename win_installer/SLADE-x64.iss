#include "include/Defines.iss"

#include "include/Setup.iss"
ArchitecturesInstallIn64BitMode=x64
OutputBaseFilename=Setup_{#MyAppName}_{#MyAppVersion}_x64

#include "include/Languages.iss"

#include "include/Tasks.iss"

[Files]
Source: "..\dist\build64\SLADE.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\build64\SLADE.pdb"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\slade.pk3"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\Tools\*"; DestDir: "{app}\Tools"; Flags: ignoreversion recursesubdirs createallsubdirs

#include "include/Icons.iss"

#include "include/Run.iss"
