# Generates dist/res/contributors.txt (shown in the About dialog) from the list
# of GitHub contributors to the SLADE repository, ordered by first contribution

$repo = "sirjuddington/SLADE"
$outfile = Join-Path $PSScriptRoot "..\dist\res\contributors.txt"

$headers = @{ "User-Agent" = "SLADE-updatecontributors" }
if ($env:GITHUB_TOKEN) { $headers["Authorization"] = "Bearer $env:GITHUB_TOKEN" }

Write-Host "Retrieving contributors from GitHub..." -ForegroundColor Blue

$contributors = @()
$page = 1
try
{
	do
	{
		$url = "https://api.github.com/repos/$repo/contributors?per_page=100&page=$page"
		# Need to unroll the array (powershell returns it as a single object)
		$result = @(Invoke-RestMethod -Uri $url -Headers $headers | ForEach-Object { $_ })
		$contributors += $result | Where-Object { $_.type -eq "User" -and $_.login -ne "sirjuddington" }
		$page++
	}
	while ($result.Count -eq 100)
}
catch
{
	Write-Host "Failed to retrieve contributors: $_" -ForegroundColor Red
	Write-Host "Keeping existing contributors.txt" -ForegroundColor Yellow
	return
}

if ($contributors.Count -eq 0)
{
	Write-Host "No contributors returned, keeping existing contributors.txt" -ForegroundColor Yellow
	return
}

# Only add contributors not already in contributors.txt (existing order is kept)
$existing = @()
if (Test-Path $outfile)
{
	$existing = @(Get-Content $outfile | Where-Object { $_.Trim() -ne "" } | ForEach-Object { $_.Trim() })
}
$contributors = @($contributors | Where-Object { $existing -notcontains $_.login })
if ($contributors.Count -eq 0)
{
	Write-Host "No new contributors, contributors.txt is up to date" -ForegroundColor Green
	return
}
Write-Host "$($contributors.Count) new contributor(s) found" -ForegroundColor Blue

# Order new contributors by date of first commit (earliest first). This needs 2
# API requests per contributor, so a GITHUB_TOKEN env var is needed to avoid the
# unauthenticated rate limit. If it fails, the commit count order is kept.
function Get-FirstCommitDate($login)
{
	$base = "https://api.github.com/repos/$repo/commits?author=$login&per_page=1"
	$response = Invoke-WebRequest -Uri $base -Headers $headers -UseBasicParsing
	$link = $response.Headers["Link"]
	if ($link -and $link -match '[?&]page=(\d+)>; rel="last"')
	{
		$response = Invoke-WebRequest -Uri "$base&page=$($Matches[1])" -Headers $headers -UseBasicParsing
	}
	$commits = @($response.Content | ConvertFrom-Json | ForEach-Object { $_ })
	$date = if ($commits.Count -gt 0) { $commits[0].commit.author.date } else { $null }
	if (-not $date)
	{
		# GitHub didn't link any commits to this login, so try the local git
		# history (match the login against commit author name/email)
		$local = git -C $PSScriptRoot log --all -i "--author=$login" --reverse --format=%aI 2>$null | Select-Object -First 1
		if ($local) { $date = $local }
	}
	if (-not $date)
	{
		# Can't determine, sort to the end
		Write-Host "No commit date found for $login" -ForegroundColor Yellow
		return [datetime]::MaxValue
	}
	return [datetime]$date
}

Write-Host "Determining first contribution dates..." -ForegroundColor Blue
try
{
	$dated = foreach ($c in $contributors)
	{
		[pscustomobject]@{ login = $c.login; first = (Get-FirstCommitDate $c.login) }
	}
	$contributors = @($dated | Sort-Object first)
}
catch
{
	Write-Host "Failed to determine first contribution dates (set GITHUB_TOKEN?): $_" -ForegroundColor Yellow
	Write-Host "Using commit count order instead" -ForegroundColor Yellow
}

# Write as UTF-8 with new contributors appended
$all  = @($existing) + @($contributors | ForEach-Object { $_.login })
$text = ($all -join "`n") + "`n"
[System.IO.File]::WriteAllText([System.IO.Path]::GetFullPath($outfile), $text, (New-Object System.Text.UTF8Encoding($false)))

Write-Host "Added $($contributors.Count) contributor(s), $($all.Count) total in contributors.txt" -ForegroundColor Green
