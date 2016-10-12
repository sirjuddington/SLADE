// requires Windows 7, Windows 7 Service Pack 1, Windows Server 2003 Service Pack 2, Windows Server 2008, Windows Server 2008 R2, Windows Server 2008 R2 SP1, Windows Vista Service Pack 1, Windows XP Service Pack 3
// requires Windows Installer 3.1 or later
// requires Internet Explorer 5.01 or later
// http://www.microsoft.com/downloads/en/details.aspx?FamilyID=9cfb2d51-5ff4-4491-b0e5-b386f32c0992

[CustomMessages]
vcredist2015_title=Visual C++ 2015 Redistributable

en.vcredist2015_size=6.2 MB

en.vcredist2015_size_x64=6.8 MB

en.vcredist2015_size_ia64=6.8 MB

;http://www.microsoft.com/globaldev/reference/lcid-all.mspx
en.vcredist2015_lcid=''


[Code]
const
	vcredist2015_url = 'http://slade.mancubus.net/files/vcredist/2015U2/vc_redist.x86.exe';
	vcredist2015_url_x64 = 'http://download.microsoft.com/download/C/E/5/CE514EAE-78A8-4381-86E8-29108D78DBD4/VC_redist.x64.exe';
	vcredist2015_url_ia64 = 'http://download.microsoft.com/download/C/E/5/CE514EAE-78A8-4381-86E8-29108D78DBD4/VC_redist.x64.exe';

procedure vcredist2015();
var
	version: cardinal;
begin
//	RegQueryDWordValue(HKLM, 'SOFTWARE\Microsoft\VisualStudio\11.0\VC\VCRedist\' + GetString('x86', 'x64', 'ia64'), 'Install', version);

	if not (FileExists('C:\Windows\System32\msvcp140.dll')) then
		AddProduct('vcredist2015' + GetArchitectureString() + '.exe',
			CustomMessage('vcredist2015_lcid') + '/passive /norestart',
			CustomMessage('vcredist2015_title'),
			CustomMessage('vcredist2015_size' + GetArchitectureString()),
			GetString(vcredist2015_url, vcredist2015_url, vcredist2015_url),
			false, false);
end;