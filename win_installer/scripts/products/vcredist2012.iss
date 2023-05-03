// requires Windows 7, Windows 7 Service Pack 1, Windows Server 2003 Service Pack 2, Windows Server 2008, Windows Server 2008 R2, Windows Server 2008 R2 SP1, Windows Vista Service Pack 1, Windows XP Service Pack 3
// requires Windows Installer 3.1 or later
// requires Internet Explorer 5.01 or later
// http://www.microsoft.com/downloads/en/details.aspx?FamilyID=9cfb2d51-5ff4-4491-b0e5-b386f32c0992

[CustomMessages]
vcredist2012_title=Visual C++ 2012 Redistributable
ru.vcredist2012_title=Распространяемые пакеты Visual C++ 2012

en.vcredist2012_size=6.3 MB
ru.vcredist2012_size=6,3 МБ

en.vcredist2012_size_x64=7.0 MB
ru.vcredist2012_size_x64=7,0 МБ

en.vcredist2012_size_ia64=7.0 MB
ru.vcredist2012_size_ia64=7,0 МБ

;http://www.microsoft.com/globaldev/reference/lcid-all.mspx
en.vcredist2012_lcid=''


[Code]
const
	vcredist2012_url = 'http://download.microsoft.com/download/1/6/B/16B06F60-3B20-4FF2-B699-5E9B7962F9AE/VSU3/vcredist_x86.exe';
	vcredist2012_url_x64 = 'http://download.microsoft.com/download/1/6/B/16B06F60-3B20-4FF2-B699-5E9B7962F9AE/VSU3/vcredist_x64.exe';
	vcredist2012_url_ia64 = 'http://download.microsoft.com/download/1/6/B/16B06F60-3B20-4FF2-B699-5E9B7962F9AE/VSU3/vcredist_x64.exe';

procedure vcredist2012();
var
	version: cardinal;
begin
//	RegQueryDWordValue(HKLM, 'SOFTWARE\Microsoft\VisualStudio\11.0\VC\VCRedist\' + GetString('x86', 'x64', 'ia64'), 'Install', version);

	if not (FileExists('C:\Windows\System32\msvcr110.dll')) then
		AddProduct('vcredist2012' + GetArchitectureString() + '.exe',
			CustomMessage('vcredist2012_lcid') + '/passive /norestart',
			CustomMessage('vcredist2012_title'),
			CustomMessage('vcredist2012_size' + GetArchitectureString()),
			GetString(vcredist2012_url, vcredist2012_url_x64, vcredist2012_url_ia64),
			false, false);
end;