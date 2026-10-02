param([switch]$ConfirmReview)
$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)
$OutputEncoding = [Console]::OutputEncoding
$dashboardRoot = $PSScriptRoot
$planRoot = Split-Path -Parent $dashboardRoot
$projectRoot = Split-Path -Parent $planRoot
$reviewPath = Join-Path $dashboardRoot 'review.json'
$dataPath = Join-Path $dashboardRoot 'dashboard-data.js'

function Invoke-ProjectGit {
    param([string[]]$GitArguments)
    $gitPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $result = @(& git -C $projectRoot -c core.quotepath=false @GitArguments 2>$null)
        $gitExit = $LASTEXITCODE
    } catch { return @() }
    finally { $ErrorActionPreference = $gitPreference }
    if ($gitExit -ne 0) { return @() }
    return $result
}

$review = [IO.File]::ReadAllText($reviewPath, [Text.Encoding]::UTF8) | ConvertFrom-Json
$head = @(Invoke-ProjectGit @('rev-parse', 'HEAD')) -join ''
$branch = @(Invoke-ProjectGit @('branch', '--show-current')) -join ''
$now = [DateTimeOffset]::UtcNow.ToOffset([TimeSpan]::FromHours(9))
$previousHead = ''
if (Test-Path -LiteralPath $dataPath) {
    try {
        $previousText = [IO.File]::ReadAllText($dataPath, [Text.Encoding]::UTF8)
        $previous = $previousText.Substring('window.PADO_DATA = '.Length).Trim().TrimEnd(';') | ConvertFrom-Json
        $previousHead = $previous.meta.head
    } catch { $previousHead = '' }
}

$documents = @()
$sourceHashes = [ordered]@{}
$files = @()
Get-ChildItem -LiteralPath $planRoot -Recurse -File | Where-Object {
    -not $_.FullName.StartsWith($dashboardRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)
} | Sort-Object FullName | ForEach-Object {
    $relative = $_.FullName.Substring($planRoot.Length + 1).Replace('\', '/')
    $files += $relative
    if ($_.Extension -eq '.md') {
        # ReadAllText avoids Get-Content's extended PSDrive metadata, which Windows
        # PowerShell 5.1 would recursively serialize with ConvertTo-Json.
        $content = [IO.File]::ReadAllText($_.FullName, [Text.Encoding]::UTF8)
        $titleMatch = [regex]::Match($content, '(?m)^#\s+(.+)$')
        $title = if ($titleMatch.Success) { $titleMatch.Groups[1].Value.Trim() } else { $_.BaseName }
        $group = if ($relative.Contains('/')) { $relative.Split('/')[0] } else { '안내' }
        $hash = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
        $sourceHashes[$relative] = $hash
        $documents += [ordered]@{ path = $relative; title = $title; group = $group; content = $content; hash = $hash }
    }
}

if ($ConfirmReview) {
    $review.reviewedAt = $now.ToString('yyyy-MM-dd')
    $review.reviewedHead = $head
    $review.sourceHashes = [pscustomobject]$sourceHashes
    $review | ConvertTo-Json -Depth 30 | Set-Content -LiteralPath $reviewPath -Encoding UTF8
}

$changedSources = @()
foreach ($key in $sourceHashes.Keys) {
    $old = $review.sourceHashes.PSObject.Properties[$key]
    if (-not $old -or $old.Value -ne $sourceHashes[$key]) { $changedSources += $key }
}
foreach ($old in $review.sourceHashes.PSObject.Properties) {
    if (-not $sourceHashes.Contains($old.Name)) { $changedSources += $old.Name }
}
$sinceReview = @()
if ($review.reviewedHead -and $head -and $head -ne $review.reviewedHead) {
    $sinceReview = @(Invoke-ProjectGit @('diff', '--name-only', ($review.reviewedHead + '..' + $head), '--', 'Source', 'Config', 'Content', 'Plan')) |
        Where-Object { $_ -notlike 'Plan/Dashboard/*' -and $_ -ne 'Plan/README.md' }
}
$sinceRefresh = @()
if ($previousHead -and $head -and $head -ne $previousHead) {
    $sinceRefresh = @(Invoke-ProjectGit @('diff', '--name-only', ($previousHead + '..' + $head), '--', 'Source', 'Config', 'Content', 'Plan')) |
        Where-Object { $_ -notlike 'Plan/Dashboard/*' }
}
$commits = @()
foreach ($line in @(Invoke-ProjectGit @('log', '-8', '--date=short', '--format=%H%x1f%h%x1f%ad%x1f%an%x1f%s', '--', 'Source', 'Config', 'Content', 'Plan', ':(exclude)Plan/Dashboard'))) {
    $parts = $line.Split([char]31)
    if ($parts.Count -ne 5) { continue }
    $commitFiles = @(Invoke-ProjectGit @('show', '--format=', '--name-only', $parts[0])) | Where-Object { $_ -and $_ -notlike 'Plan/Dashboard/*' }
    $commits += [ordered]@{ hash = $parts[0]; short = $parts[1]; date = $parts[2]; author = $parts[3]; subject = $parts[4]; files = @($commitFiles) }
}
$working = @(Invoke-ProjectGit @('status', '--short', '--untracked-files=normal', '--', 'Source', 'Config', 'Content', 'Plan')) |
    # Markdown changes are already compared to the reviewed source hashes above.
    # A reviewed document need not be committed before its summary is current.
    Where-Object { $_ -notmatch 'Plan/Dashboard/' -and $_ -notmatch 'Plan/README.md$' -and $_ -notmatch 'Plan/.+\.md"?$' }

$tests = @()
$testDoc = $documents | Where-Object { $_.path -eq '공용/08_1단계_통합_검증표.md' } | Select-Object -First 1
if ($testDoc) {
    foreach ($line in ($testDoc.content -split '\r?\n')) {
        if ($line -match '^\|\s*(V\d+)\s*\|') {
            $cells = @($line.Trim().Trim('|').Split('|') | ForEach-Object { $_.Trim() })
            if ($cells.Count -ge 6) { $tests += [ordered]@{ id = $cells[0]; players = $cells[1]; steps = $cells[2]; expected = $cells[3]; owner = $cells[4]; result = $cells[5] } }
        }
    }
}
$payload = [ordered]@{
    meta = [ordered]@{ generatedAt = $now.ToString('yyyy-MM-dd HH:mm'); timezone = 'Asia/Seoul'; head = $head; branch = $branch; previousHead = $previousHead; changedSources = @($changedSources); sinceReview = @($sinceReview); sinceRefresh = @($sinceRefresh); working = @($working); initial = -not [bool]$previousHead; gitAvailable = [bool]$head }
    review = $review
    documents = @($documents)
    files = @($files)
    commits = @($commits)
    tests = @($tests)
}
$json = $payload | ConvertTo-Json -Depth 40 -Compress
# A classic script works both from file:// and from a local server; no fetch or CDN is needed.
[IO.File]::WriteAllText($dataPath, ('window.PADO_DATA = ' + $json + ';'), [Text.UTF8Encoding]::new($false))
Write-Output ('갱신 완료: 문서 {0}개, 검증 항목 {1}개. 검토 이후 변경 문서 {2}개.' -f $documents.Count, $tests.Count, $changedSources.Count)
