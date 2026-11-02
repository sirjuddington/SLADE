#include "include/Defines.iss"

#include "include/Setup.iss"
OutputBaseFilename={#MyAppName}_{#MyAppVersion}_win_x86_setup

#include "include/Languages.iss"

#include "include/Tasks.iss"

[Files]
Source: "..\dist\build32\SLADE.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\build32\SLADE.pdb"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\slade.pk3"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\Tools\*"; DestDir: "{app}\Tools"; Flags: ignoreversion recursesubdirs createallsubdirs

#include "include/Icons.iss"

#include "include/Run.iss"
