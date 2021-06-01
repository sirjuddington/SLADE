$version = "3.2.0_b3"
$rev_short = Invoke-Expression "git.exe rev-parse --short HEAD"

# Check for 7-zip install
$7zpath = "$env:ProgramFiles\7-Zip\7z.exe"
if (-not (Test-Path $7zpath))
{
	Write-Host "7-zip is required to make slade.pk3 and the release binaries archive" -foregroundcolor red
}

# Prompt to use git revision
Write-Host "`nUse git revision in version? ($rev_short) (y/n) " -foregroundcolor cyan -nonewline
$userev = Read-Host
if ($userev.ToLower() -eq "y")
{
	$env:CL = "/DGIT_DESCRIPTION=`"\`"$rev_short\`"`""
	$version = "${version}_$rev_short"
}

# Prompt to build SLADE
Write-Host "`nRebuild SLADE? (y/n) " -foregroundcolor cyan -nonewline
$buildbinaries = Read-Host

# Build SLADE
if ($buildbinaries.ToLower() -eq "y")
{
	# Find devenv path (community or professional)
	$devenvpath19 = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Professional\Common7\IDE\devenv.com"
	if (-not (Test-Path $devenvpath19))
	{
		$devenvpath19 = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Community\Common7\IDE\devenv.com"
	}
	if (-not (Test-Path $devenvpath19))
	{
		Write-Host "`nCould not find Visual Studio 2019 path" -ForegroundColor Red
		Exit-PSSession
	}
	else
	{
		& $devenvpath19 (resolve-path ..\msvc\SLADE.sln).Path /rebuild "Release|Win32" /project SLADE.vcxproj
	    & $devenvpath19 (resolve-path ..\msvc\SLADE.sln).Path /rebuild "Release|x64" /project SLADE.vcxproj
	}
}

# Determine release directory + platforms
$releasedir = "$PSScriptRoot\$version"
$releasedir32 = "$releasedir\win32"
$releasedir64 = "$releasedir\x64"

# Create release directory if needed
Write-Host "`nCreate directory $releasedir" -foregroundcolor yellow
New-Item -ItemType directory -Force -Path "$releasedir32" | out-null
New-Item -ItemType directory -Force -Path "$releasedir64" | out-null

# Remove existing pk3 if it exists
$pk3path = ".\slade.pk3"
if (Test-Path "$pk3path")
{
	Write-Host "`nRemoving existing slade.pk3" -foregroundcolor yellow
	Remove-Item "$pk3path"
}

# Clean out Thumbs.db files from res folder
Write-Host "`nRemoving Thumbs.db files..." -foregroundcolor yellow
$resdir = (resolve-path ".\res").path
Get-ChildItem -Path . -Include Thumbs.db -Recurse -Name -Force | Remove-Item -Force

# Build pk3
Write-Host "`nBuilding slade.pk3..." -foregroundcolor yellow
& $7zpath a -tzip $pk3path "$resdir\*" | out-null
Write-Host "Done" -foregroundcolor green

# Copy Files
Write-Host "`nCopying SLADE files..." -foregroundcolor yellow
# Common
Copy-Item (resolve-path ".\slade.pk3") "$releasedir" -Force
# Win32
Copy-Item (resolve-path ".\SLADE.exe")               "$releasedir32" -Force
Copy-Item (resolve-path ".\SLADE.pdb")               "$releasedir32" -Force
Copy-Item (resolve-path ".\dll32\libfluidsynth.dll") "$releasedir32" -Force
# x64
Copy-Item (resolve-path ".\SLADE-x64.exe")       "$releasedir64\SLADE.exe" -Force
Copy-Item (resolve-path ".\SLADE-x64.pdb")       "$releasedir64\SLADE.pdb" -Force
Write-Host "Done" -foregroundcolor green

# Prompt to build binaries 7z
Write-Host "`nBuild Binaries 7z? (y/n) " -foregroundcolor cyan -nonewline
$buildbinaries = Read-Host

# Build binaries 7z
if ($buildbinaries.ToLower() -eq "y")
{
	# Append timestamp to filenames if git rev is used
	$timestamp = Get-Date -Format FileDate
	if ($userev.ToLower() -eq "y") {
		$timestamp = "_${timestamp}"
	}
	else {
		$timestamp = ""
	}

	Write-Host "`nBuiling win32 binaries 7z..." -foregroundcolor yellow
	& $7zpath a -t7z "$releasedir\slade_${version}${timestamp}.7z" `
	"$releasedir32\libfluidsynth.dll" `
	"$releasedir32\SLADE.exe" `
	"$releasedir32\SLADE.pdb" `
	"$releasedir\slade.pk3"
	Write-Host "Done" -foregroundcolor green

	Write-Host "`nBuiling x64 binaries 7z..." -foregroundcolor yellow
	& $7zpath a -t7z "$releasedir\slade_${version}_x64${timestamp}.7z" `
	"$releasedir64\SLADE.exe" `
	"$releasedir64\SLADE.pdb" `
	"$releasedir\slade.pk3"
	Write-Host "Done" -foregroundcolor green
}

# Prompt to build installer
Write-Host "`nBuild Installer? (y/n) " -foregroundcolor cyan -nonewline
$buildinstaller = Read-Host

# Build installer
if ($buildinstaller.ToLower() -eq "y")
{
	$innocompiler = "${env:ProgramFiles(x86)}\Inno Setup 5\iscc.exe"
	if (-not (Test-Path $innocompiler))
	{
		$innocompiler = "${env:ProgramFiles}\Inno Setup 5\iscc.exe"
	}
	if (Test-Path $innocompiler)
	{
		Write-Host "`nBuiling installer..." -foregroundcolor yellow
		& $innocompiler "/Q" "/O$releasedir" "/F`"Setup_SLADE_$version`"" (resolve-path "..\win_installer\SLADE.iss").Path
		Write-Host "Done" -foregroundcolor green
	}
}

# Done
Write-Host "`nFinished making release, press any key to exit" -foregroundcolor green
$x = $host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
