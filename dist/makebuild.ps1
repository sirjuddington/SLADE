$version = "3110_b3"
$versionstring = "3.1.1 Beta 3"

# Check for 7-zip install
$7zpath = "$env:ProgramFiles\7-Zip\7z.exe"
if (-not (Test-Path $7zpath))
{
    Write-Host "7-zip is required to make slade.pk3 and the release binaries archive" -foregroundcolor red
}

# Prompt for version number
#$version = Read-Host -Prompt "Enter a version number (eg. 3110)"

# Prompt for beta number
#$beta = Read-Host -Prompt "Enter a beta number (blank for none)"

# Determine full version string
#$versionstring = $version
#if ($beta)
#{
#    $versionstring = $version+"_b"+$beta
#}

# Determine release directory
$releasedir = "$PSScriptRoot\$version"

# Create release directory if needed
Write-Host "`nCreate directory $releasedir" -foregroundcolor yellow
New-Item -ItemType directory -Force -Path $releasedir | out-null

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
Copy-Item (resolve-path ".\FreeImage.dll")      "$releasedir" -Force
Copy-Item (resolve-path ".\libfluidsynth.dll")  "$releasedir" -Force
Copy-Item (resolve-path ".\openal32.dll")       "$releasedir" -Force
Copy-Item (resolve-path ".\SLADE.exe")          "$releasedir" -Force
Copy-Item (resolve-path ".\SLADE.pdb")          "$releasedir" -Force
Copy-Item (resolve-path ".\slade.pk3")          "$releasedir" -Force
Copy-Item (resolve-path ".\slade3.txt")         "$releasedir" -Force
Write-Host "Done" -foregroundcolor green

# Prompt to build binaries 7z
Write-Host "`nBuild Binaries 7z? (y/n) " -foregroundcolor cyan -nonewline
$buildbinaries = Read-Host

# Build binaries 7z
if ($buildbinaries.ToLower() -eq "y")
{
    Write-Host "`nBuiling binaries 7z..." -foregroundcolor yellow
    & $7zpath a -t7z "$releasedir\slade_$version.7z" `
    "$releasedir\FreeImage.dll" `
    "$releasedir\libfluidsynth.dll" `
    "$releasedir\openal32.dll" `
    "$releasedir\SLADE.exe" `
    "$releasedir\SLADE.pdb" `
    "$releasedir\slade3.txt" `
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
        & $innocompiler "/Q" "/O$releasedir" "/F`"Setup_SLADE_$versionstring`"" (resolve-path "..\win_installer\SLADE.iss").Path
        Write-Host "Done" -foregroundcolor green
    }
}

# Done
Write-Host "`nFinished making release, press any key to exit" -foregroundcolor green
$x = $host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
