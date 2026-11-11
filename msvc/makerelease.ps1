$version = "3.3.1"
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
if ($buildbinaries.ToLower() -eq "y")
{
	& ./buildall.ps1
}

# Determine release directory + platforms
$releasedir = "..\dist\$version"
$releasedir64 = "$releasedir\x64"

# Create release directory if needed
Write-Host "`nCreate directory $releasedir" -foregroundcolor blue
New-Item -ItemType directory -Force -Path "$releasedir64" | out-null

# Remove existing pk3 if it exists
$pk3path = "..\dist\slade.pk3"
if (Test-Path "$pk3path")
{
	Write-Host "`nRemoving existing slade.pk3" -foregroundcolor blue
	Remove-Item "$pk3path"
}

# Clean out Thumbs.db files from res folder
Write-Host "`nRemoving Thumbs.db files..." -foregroundcolor blue
$resdir = (resolve-path "..\dist\res").path
Get-ChildItem -Path ..\dist -Include Thumbs.db -Recurse -Name -Force | Remove-Item -Force

# Build pk3
Write-Host "`nBuilding slade.pk3..." -foregroundcolor blue
& $7zpath a -tzip $pk3path "$resdir\*" | out-null
Write-Host "Done" -foregroundcolor green

# Copy Files
Write-Host "`nCopying SLADE files..." -foregroundcolor blue
Copy-Item (resolve-path "..\dist\slade.pk3") "$releasedir" -Force
Copy-Item (resolve-path "..\dist\build64\SLADE.exe") "$releasedir64" -Force
Copy-Item (resolve-path "..\dist\build64\SLADE.pdb") "$releasedir64" -Force
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

	Write-Host "`nBuiling x64 binaries 7z..." -foregroundcolor blue
	& $7zpath a -t7z "$releasedir\SLADE_${version}${timestamp}_win_x64.7z" `
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
	$innocompiler = "${env:ProgramFiles(x86)}\Inno Setup 6\iscc.exe"
	if (-not (Test-Path $innocompiler))
	{
		$innocompiler = "${env:ProgramFiles}\Inno Setup 6\iscc.exe"
	}
	if (Test-Path $innocompiler)
	{
		Write-Host "`nBuiling x64 installer..." -foregroundcolor blue
		& $innocompiler "/O+" "/O$releasedir" (resolve-path "..\win_installer\SLADE-x64.iss").Path
		Write-Host "Done" -foregroundcolor green
	}
}

# Done
Write-Host "`nFinished making release, press any key to exit" -foregroundcolor green
$x = $host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
