# Current version
$version_major =    "3"
$version_minor =    "2"
$version_revision = "9"
$version_beta =     "0"

# Prompt for new version numbers
Write-Host "Major version number: $version_major (locked)"
if ($v = Read-Host "Enter minor version number (current $version_minor)") { $version_minor = $v }
if ($v = Read-Host "Enter revision number (current $version_revision)") { $version_revision = $v }
if ($v = Read-Host "Enter beta number (0 for release, current $version_beta)") { $version_beta = $v }


# App.cpp ----------------------------------------------------------------------

$file = "../src/Application/App.cpp"
$version_line = Get-Content $file `
	| Select-String "Version version_num" `
	| Select-Object -ExpandProperty Line
$new_version_line = "Version version_num`{ $version_major, $version_minor, $version_revision, $version_beta `};"

if ($null -eq $version_line)
{
	Write-Host "Version line not found in $file" -ForegroundColor Red
}
else
{
	Write-Host "Updating version in $file" -ForegroundColor Blue
	Write-Host "From: $version_line" -ForegroundColor DarkGray
	Write-Host "To:   $new_version_line" -ForegroundColor DarkGray
	(Get-Content $file) -replace $version_line, $new_version_line | Set-Content $file
}

# makerelase.ps1 ---------------------------------------------------------------

$file = "makerelease.ps1"
$version_line = Get-Content $file `
	| Select-String "version = " `
	| Select-Object -ExpandProperty Line -First 1
$version_line = $version_line.substring(1) # Remove leading $
if ($version_beta -eq "0")
{
	$new_version_line = "version = `"$version_major.$version_minor.$version_revision`""
}
else
{
	$new_version_line = "version = `"${version_major}.${version_minor}.${version_revision}_b${version_beta}`""
}

if ($null -eq $version_line)
{
	Write-Host "Version line not found in $file" -ForegroundColor Red
}
else
{
	Write-Host "Updating version in $file" -ForegroundColor Blue
	Write-Host "From: $version_line" -ForegroundColor DarkGray
	Write-Host "To:   $new_version_line" -ForegroundColor DarkGray

	(Get-Content $file) -replace $version_line, $new_version_line | Set-Content $file
}

# SLADE.rc ---------------------------------------------------------------------

$file = "./SLADE.rc"
$version_line1 = Get-Content $file `
	| Select-String "#define SLADE_VERSION " `
	| Select-Object -ExpandProperty Line
$version_line2 = Get-Content $file `
	| Select-String "#define SLADE_VERSION_STR " `
	| Select-Object -ExpandProperty Line
$copyright_line = Get-Content $file `
	| Select-String "#define SLADE_COPYRIGHT " `
	| Select-Object -ExpandProperty Line

if ($null -eq $version_line1 -or $null -eq $version_line2 -or $null -eq $copyright_line)
{
	Write-Host "Version lines not found in $file" -ForegroundColor Red
}
else
{
	$new_version_line1 = "#define SLADE_VERSION $version_major,$version_minor,$version_revision,0"
	if ($version_beta -eq "0")
	{
		$new_version_line2 = "#define SLADE_VERSION_STR `"$version_major.$version_minor.$version_revision`""
	}
	else
	{
		$new_version_line2 = `
			"#define SLADE_VERSION_STR `"${version_major}.${version_minor}.${version_revision} Beta ${version_beta}`""
	}
	$year = (Get-Date).Year
	$new_copyright_line = "#define SLADE_COPYRIGHT `"Copyright `(C`) 2008-$year`""

	Write-Host "Updating version in $file" -ForegroundColor Blue
	Write-Host "From: $version_line1" -ForegroundColor DarkGray
	Write-Host "To:   $new_version_line1" -ForegroundColor DarkGray
	Write-Host "From: $version_line2" -ForegroundColor DarkGray
	Write-Host "To:   $new_version_line2" -ForegroundColor DarkGray
	Write-Host "From: $copyright_line" -ForegroundColor DarkGray
	Write-Host "To:   $new_copyright_line" -ForegroundColor DarkGray

	(Get-Content $file) -replace $version_line1, $new_version_line1 | Set-Content $file
	(Get-Content $file) -replace $version_line2, $new_version_line2 | Set-Content $file
	(Get-Content $file) -replace [regex]::Escape($copyright_line), $new_copyright_line | Set-Content $file -Encoding UTF8
}

# Defines.iss ------------------------------------------------------------------

$file = "../win_installer/include/Defines.iss"
$version_line = Get-Content $file `
	| Select-String "#define MyAppVersion" `
	| Select-Object -ExpandProperty Line
if ($version_beta -eq "0")
{
	$new_version_line = "#define MyAppVersion `"$version_major.$version_minor.$version_revision`""
}
else
{
	$new_version_line = `
		"#define MyAppVersion `"${version_major}.${version_minor}.${version_revision} Beta ${version_beta}`""
}

if ($null -eq $version_line)
{
	Write-Host "Version line not found in $file" -ForegroundColor Red
}
else
{
	Write-Host "Updating version in $file" -ForegroundColor Blue
	Write-Host "From: $version_line" -ForegroundColor DarkGray
	Write-Host "To:   $new_version_line" -ForegroundColor DarkGray
	(Get-Content $file) -replace $version_line, $new_version_line | Set-Content $file
}

# net.mancubus.SLADE.metainfo.xml ----------------------------------------------

$file = "../net.mancubus.SLADE.metainfo.xml"
$release = "$version_major.$version_minor.$version_revision"
if (Get-Content $file | Select-String "version=`"$release`"")
{
	Write-Host "Release $release already exists in $file" -ForegroundColor Yellow
}
else
{
	$date = Get-Date -Format "yyyy-MM-dd"
	$new_releases_node = `
		"<releases>`r`n`t`t<release date=`"$date`" version=`"$release`"/>"

	Write-Host "Adding release in $file" -ForegroundColor Blue
	Write-Host "Text: $new_releases_node" -ForegroundColor DarkGray

	(Get-Content $file) -replace "<releases>", $new_releases_node | Set-Content $file
}

# Info.plist -------------------------------------------------------------------

$file = "../Info.plist"
$version_line = Get-Content $file `
	| Select-String "<string>3." `
	| Select-Object -ExpandProperty Line -First 1
$new_version_line = "`t<string>$version_major.$version_minor.$version_revision</string>"

if ($null -eq $version_line)
{
	Write-Host "Version line not found in $file" -ForegroundColor Red
}
else
{
	Write-Host "Updating version in $file" -ForegroundColor Blue
	Write-Host "From: $version_line" -ForegroundColor DarkGray
	Write-Host "To:   $new_version_line" -ForegroundColor DarkGray
	(Get-Content $file) -replace $version_line, $new_version_line | Set-Content $file
}

# updateversion.ps1 ------------------------------------------------------------

$file = "updateversion.ps1"
$version_minor_line = Get-Content $file `
	| Select-String "version_minor = " `
	| Select-Object -ExpandProperty Line -First 1
$version_revision_line = Get-Content $file `
	| Select-String "version_revision = " `
	| Select-Object -ExpandProperty Line -First 1
$version_beta_line = Get-Content $file `
	| Select-String "version_beta = " `
	| Select-Object -ExpandProperty Line -First 1

# Remove leading $
$version_minor_line = $version_minor_line.substring(1)
$version_revision_line = $version_revision_line.substring(1)
$version_beta_line = $version_beta_line.substring(1)

if ($null -eq $version_minor_line -or $null -eq $version_revision_line -or $null -eq $version_beta_line)
{
	Write-Host "Version lines not found in $file" -ForegroundColor Red
}
else
{
	$new_version_minor_line = "version_minor =    `"$version_minor`""
	$new_version_revision_line = "version_revision = `"$version_revision`""
	$new_version_beta_line = "version_beta =     `"$version_beta`""

	Write-Host "Updating version in $file" -ForegroundColor Blue
	Write-Host "From: $version_minor_line" -ForegroundColor DarkGray
	Write-Host "To:   $new_version_minor_line" -ForegroundColor DarkGray
	Write-Host "From: $version_revision_line" -ForegroundColor DarkGray
	Write-Host "To:   $new_version_revision_line" -ForegroundColor DarkGray
	Write-Host "From: $version_beta_line" -ForegroundColor DarkGray
	Write-Host "To:   $new_version_beta_line" -ForegroundColor DarkGray

	(Get-Content $file) -replace $version_minor_line, $new_version_minor_line | Set-Content $file
	(Get-Content $file) -replace $version_revision_line, $new_version_revision_line | Set-Content $file
	(Get-Content $file) -replace $version_beta_line, $new_version_beta_line | Set-Content $file
}
