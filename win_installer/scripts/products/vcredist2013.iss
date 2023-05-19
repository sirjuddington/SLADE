// requires Windows 7, Windows 7 Service Pack 1, Windows Server 2003 Service Pack 2, Windows Server 2008, Windows Server 2008 R2, Windows Server 2008 R2 SP1, Windows Vista Service Pack 1, Windows XP Service Pack 3
// requires Windows Installer 3.1 or later
// requires Internet Explorer 5.01 or later
// http://www.microsoft.com/downloads/en/details.aspx?FamilyID=9cfb2d51-5ff4-4491-b0e5-b386f32c0992

[CustomMessages]
vcredist2013_title=Visual C++ 2013 Redistributable
ru.vcredist2013_title=Распространяемые пакеты Visual C++ 2013

en.vcredist2013_size=6.2 MB
ru.vcredist2013_size=6,2 МБ

en.vcredist2013_size_x64=6.8 MB
ru.vcredist2013_size_x64=6,8 МБ

en.vcredist2013_size_ia64=6.8 MB
ru.vcredist2013_size_ia64=6,8 МБ

;http://www.microsoft.com/globaldev/reference/lcid-all.mspx
en.vcredist2013_lcid=''


[Code]
const
	vcredist2013_url = 'http://download.microsoft.com/download/2/E/6/2E61CFA4-993B-4DD4-91DA-3737CD5CD6E3/vcredist_x86.exe';
	vcredist2013_url_x64 = 'http://download.microsoft.com/download/2/E/6/2E61CFA4-993B-4DD4-91DA-3737CD5CD6E3/vcredist_x64.exe';
	vcredist2013_url_ia64 = 'http://download.microsoft.com/download/2/E/6/2E61CFA4-993B-4DD4-91DA-3737CD5CD6E3/vcredist_x64.exe';

procedure vcredist2013();
var
	version: cardinal;
begin
//	RegQueryDWordValue(HKLM, 'SOFTWARE\Microsoft\VisualStudio\11.0\VC\VCRedist\' + GetString('x86', 'x64', 'ia64'), 'Install', version);

	if not (FileExists('C:\Windows\System32\msvcr120.dll')) then
		AddProduct('vcredist2013' + GetArchitectureString() + '.exe',
			CustomMessage('vcredist2013_lcid') + '/passive /norestart',
			CustomMessage('vcredist2013_title'),
			CustomMessage('vcredist2013_size' + GetArchitectureString()),
			GetString(vcredist2013_url, vcredist2013_url, vcredist2013_url),
			false, false);
end;