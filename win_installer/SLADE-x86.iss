#include "include/Defines.iss"

#include "include/Setup.iss"
OutputBaseFilename=Setup_{#MyAppName}_{#MyAppVersion}

#include "include/Languages.iss"

#include "include/Tasks.iss"

[Files]
Source: "..\dist\build32\SLADE.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\build32\SLADE.pdb"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\slade.pk3"; DestDir: "{app}"; Flags: ignoreversion

#include "include/Icons.iss"

#include "include/Run.iss"
