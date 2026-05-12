# Setup -----------------------------------------------------------------------

# Find MSVC base path (community or professional)
$msvcpath = "${env:ProgramFiles}\Microsoft Visual Studio\18\Professional"
if (-not (Test-Path $msvcpath))
{
	$msvcpath = "${env:ProgramFiles}\Microsoft Visual Studio\18\Community"
}
if (-not (Test-Path $msvcpath))
{
	Write-Host "`nCould not find Visual Studio 2026 path" -ForegroundColor Red
	Exit-PSSession
}
Write-Host "`nFound VS2026 at ${msvcpath}" -foregroundcolor blue

# Check VCPKG_ROOT env var
if (-not $env:VCPKG_ROOT)
{
	Write-Host "`nVCPKG_ROOT environment variable not set!" -ForegroundColor Red
	Exit-PSSession
}

# Cmake config line vars
$buildtype = "-DCMAKE_BUILD_TYPE:STRING=`"RelWithDebInfo`""
$toolchainfile = "-DCMAKE_TOOLCHAIN_FILE=`"${env:VCPKG_ROOT}\scripts\buildsystems\vcpkg.cmake`""


# 32bit build -----------------------------------------------------------------

Write-Host "`nProceed with 32bit build? (y/n) " -foregroundcolor cyan -nonewline
$build32 = Read-Host
if ($build32.ToLower() -eq "y")
{
	# Clear existing build
	if (Test-Path "../dist/build32")
	{
		Remove-Item -Recurse -Force "../dist/build32" | out-null
	}

	# Setup 32bit build environment variables
	cmd.exe /c "call `"$msvcpath\VC\Auxiliary\Build\vcvars32.bat`" && set > %temp%\vcvars32.txt"
	Get-Content "$env:temp\vcvars32.txt" | Foreach-Object {
		if ($_ -match "^(.*?)=(.*)$")
		{
			Set-Content "env:\$($matches[1])" $matches[2]
		}
	}

	# Set 32bit cmake vars
	$targettriplet = "-DVCPKG_TARGET_TRIPLET:STRING=`"x86-windows-static`""
	$outputdir = "-DSLADE_EXE_DIR=`"dist/build32`""

	# Configure
	Write-Host "`nConfiguring 32bit build..." -foregroundcolor blue
	cmake -G Ninja $buildtype -DBUILD_PK3=OFF -DBUILD_WX=ON $targettriplet $toolchainfile $outputdir .. -B "build32"

	# Build
	Write-Host "`nBuilding 32bit executable..." -foregroundcolor blue
	cmake --build "build32"

	# Clean up
	Remove-Item -Recurse -Force "build32"
	Remove-Item -Force "$env:temp\vcvars32.txt"
}


# 64bit build -----------------------------------------------------------------

Write-Host "`nProceed with 64bit build? (y/n) " -foregroundcolor cyan -nonewline
$build64 = Read-Host
if ($build64.ToLower() -eq "y")
{
	# Clear existing build
	if (Test-Path "../dist/build64")
	{
		Remove-Item -Recurse -Force "../dist/build64" | out-null
	}

	# Setup 64bit build environment variables
	cmd.exe /c "call `"$msvcpath\VC\Auxiliary\Build\vcvars64.bat`" && set > %temp%\vcvars64.txt"
	Get-Content "$env:temp\vcvars64.txt" | Foreach-Object {
		if ($_ -match "^(.*?)=(.*)$")
		{
			Set-Content "env:\$($matches[1])" $matches[2]
		}
	}

	# Set 64bit cmake vars
	$targettriplet = "-DVCPKG_TARGET_TRIPLET:STRING=`"x64-windows-static`""
	$outputdir = "-DSLADE_EXE_DIR=`"dist/build64`""

	# Configure
	Write-Host "`nConfiguring 64bit build..." -foregroundcolor blue
	cmake -G Ninja $buildtype -DBUILD_PK3=OFF -DBUILD_WX=ON $targettriplet $toolchainfile $outputdir .. -B "build64"

	# Build
	Write-Host "`nBuilding 64bit executable..." -foregroundcolor blue
	cmake --build "build64"

	# Clean up
	Remove-Item -Recurse -Force "build64"
	Remove-Item -Force "$env:temp\vcvars64.txt"
}

Write-Host "`nDone!" -foregroundcolor green
