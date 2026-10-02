# ASCII-only source lets Windows PowerShell 5.1 read UTF-8 without BOM.
[CmdletBinding()]
param([string]$PlanRoot = '', [switch]$Check)

$ErrorActionPreference = 'Stop'
$utf8 = New-Object System.Text.UTF8Encoding($false, $true)
$lf = [string][char]10
$crlf = [string][char]13 + $lf
$config = @'
{
  "source": "\uae30\ud68d_PM/00_\uac8c\uc784\uae30\ud68d.md",
  "tests": "\uacf5\uc6a9/08_1\ub2e8\uacc4_\ud1b5\ud569_\uac80\uc99d\ud45c.md",
  "bossHeading": "\ubcf4\uc2a4 \uaddc\uce59 \ubbf8\uacb0\uc815",
  "roles": [
    {
      "key": "core",
      "path": "\ub124\ud2b8\uc6cc\ud06c_\ucf54\uc5b4/03_\ub124\ud2b8\uc6cc\ud06c_\ucf54\uc5b4.md",
      "aliases": [
        "\ub124\ud2b8\uc6cc\ud06c",
        "\ucf54\uc5b4"
      ],
      "sections": [
        "\uac8c\uc784 \uac1c\uc694\uc640 \uc138\uacc4",
        "\ud0dd\ubc30\u00b7\uc0ac\ub9dd\u00b7\ubd80\ud65c",
        "\ubcf4\uc2a4 \uc804\ud22c\uc640 \uc2e4\ud328",
        "\ubcf4\uc2a4 \uaddc\uce59 \ubbf8\uacb0\uc815",
        "\uc5ed\ud560\uacfc \uacf5\ub3d9 \uacc4\uc57d",
        "\ub2e8\uacc4\ubcc4 \uac1c\ubc1c \ubc94\uc704",
        "\ub0a8\uc740 \uacb0\uc815",
        "\ubcf4\uc2a4 \ubc30\ub2ec \uc0c1\ud0dc",
        "\ubc30\ub2ec \uc120\ud0dd\uacfc \ubcf4\uc2a4 \uc9c4\ud589",
        "\ucf58\ud150\uce20 \ubaa9\ub85d\uacfc \ubc30\uc1a1\uc9c0 \uc694\uccad",
        "\ubc29\u00b7\uc800\uc7a5\u00b7\ud569\ub958",
        "\uc2dc\uac04 \uc81c\ud55c\uacfc \ubcf4\uc0c1",
        "\uc77c\ubc18 \ubc30\ub2ec \uc0c1\ud0dc",
        "\uacbd\uc81c\uc640 \ubc38\ub7f0\uc2a4",
        "\ucc28\ub7c9 \uc561\uc158\uacfc \uc131\uc7a5",
        "\ucc28\ub7c9 \uc0c1\ud638\uc791\uc6a9\u00b7\ud53c\ud574 \uaddc\uce59",
        "\uc77c\ubc18 \uc880\ube44 \ud0c0\uae43\u00b7\uac70\ub9ac \uad00\ub9ac",
        "\ubc29\ubcc4 \uc800\uc7a5 \uaddc\uce59",
        "\ud654\uba74\uc5d0 \uc804\ub2ec\ud560 \uc0c1\ud0dc\uc640 \ubb38\uad6c"
      ],
      "stageName": "\ub124\ud2b8\uc6cc\ud06c \ucf54\uc5b4"
    },
    {
      "key": "level",
      "path": "\ub808\ubca8\ub514\uc790\uc774\ub108/02_\ub808\ubca8\ub514\uc790\uc774\ub108.md",
      "aliases": [
        "\ub808\ubca8",
        "\ub808\ubca8\ub514\uc790\uc774\ub108"
      ],
      "sections": [
        "\uac8c\uc784 \uac1c\uc694\uc640 \uc138\uacc4",
        "\ud0dd\ubc30\u00b7\uc0ac\ub9dd\u00b7\ubd80\ud65c",
        "\ubcf4\uc2a4 \uc804\ud22c\uc640 \uc2e4\ud328",
        "\ubcf4\uc2a4 \uaddc\uce59 \ubbf8\uacb0\uc815",
        "\uc5ed\ud560\uacfc \uacf5\ub3d9 \uacc4\uc57d",
        "\ub2e8\uacc4\ubcc4 \uac1c\ubc1c \ubc94\uc704",
        "\ub0a8\uc740 \uacb0\uc815",
        "\ubcf4\uc2a4 \ubc30\ub2ec \uc0c1\ud0dc",
        "\ubc30\ub2ec \uc120\ud0dd\uacfc \ubcf4\uc2a4 \uc9c4\ud589",
        "\ucf58\ud150\uce20 \ubaa9\ub85d\uacfc \ubc30\uc1a1\uc9c0 \uc694\uccad",
        "\uc77c\ubc18 \uc880\ube44 \ud0c0\uae43\u00b7\uac70\ub9ac \uad00\ub9ac",
        "\ubc29\ubcc4 \uc800\uc7a5 \uaddc\uce59",
        "\uc218\ucde8\uc778\u00b7\uc774\uc57c\uae30\u00b7\uacb0\ub9d0 \uc5f0\ucd9c\uc548",
        "\uacbd\uc81c\uc640 \ubc38\ub7f0\uc2a4"
      ],
      "stageName": "\ub808\ubca8\ub514\uc790\uc774\ub108"
    },
    {
      "key": "ai",
      "path": "AI_\ud504\ub85c\uadf8\ub798\uba38/04_AI_\ud504\ub85c\uadf8\ub798\uba38.md",
      "aliases": [
        "AI",
        "AI \ud504\ub85c\uadf8\ub798\uba38"
      ],
      "sections": [
        "\uac8c\uc784 \uac1c\uc694\uc640 \uc138\uacc4",
        "\ud0dd\ubc30\u00b7\uc0ac\ub9dd\u00b7\ubd80\ud65c",
        "\ubcf4\uc2a4 \uc804\ud22c\uc640 \uc2e4\ud328",
        "\ubcf4\uc2a4 \uaddc\uce59 \ubbf8\uacb0\uc815",
        "\uc5ed\ud560\uacfc \uacf5\ub3d9 \uacc4\uc57d",
        "\ub2e8\uacc4\ubcc4 \uac1c\ubc1c \ubc94\uc704",
        "\ub0a8\uc740 \uacb0\uc815",
        "\ubcf4\uc2a4 \ubc30\ub2ec \uc0c1\ud0dc",
        "\ucc28\ub7c9 \uc561\uc158\uacfc \uc131\uc7a5",
        "\ucc28\ub7c9 \uc0c1\ud638\uc791\uc6a9\u00b7\ud53c\ud574 \uaddc\uce59",
        "\uc77c\ubc18 \uc880\ube44 \ud0c0\uae43\u00b7\uac70\ub9ac \uad00\ub9ac",
        "\uc218\ucde8\uc778\u00b7\uc774\uc57c\uae30\u00b7\uacb0\ub9d0 \uc5f0\ucd9c\uc548"
      ],
      "stageName": "AI \ud504\ub85c\uadf8\ub798\uba38"
    },
    {
      "key": "combat",
      "path": "\ud50c\ub808\uc774\uc5b4_\uc804\ud22c/05_\ud50c\ub808\uc774\uc5b4_\uc804\ud22c.md",
      "aliases": [
        "\uc804\ud22c",
        "\ud50c\ub808\uc774\uc5b4"
      ],
      "sections": [
        "\uac8c\uc784 \uac1c\uc694\uc640 \uc138\uacc4",
        "\ud0dd\ubc30\u00b7\uc0ac\ub9dd\u00b7\ubd80\ud65c",
        "\ubcf4\uc2a4 \uc804\ud22c\uc640 \uc2e4\ud328",
        "\ubcf4\uc2a4 \uaddc\uce59 \ubbf8\uacb0\uc815",
        "\uc5ed\ud560\uacfc \uacf5\ub3d9 \uacc4\uc57d",
        "\ub2e8\uacc4\ubcc4 \uac1c\ubc1c \ubc94\uc704",
        "\ub0a8\uc740 \uacb0\uc815",
        "\ubcf4\uc2a4 \ubc30\ub2ec \uc0c1\ud0dc",
        "\ubc29\u00b7\uc800\uc7a5\u00b7\ud569\ub958",
        "\uc2dc\uac04 \uc81c\ud55c\uacfc \ubcf4\uc0c1",
        "\uc77c\ubc18 \ubc30\ub2ec \uc0c1\ud0dc",
        "\uacbd\uc81c\uc640 \ubc38\ub7f0\uc2a4",
        "\ucc28\ub7c9 \uc561\uc158\uacfc \uc131\uc7a5",
        "\ucc28\ub7c9 \uc0c1\ud638\uc791\uc6a9\u00b7\ud53c\ud574 \uaddc\uce59",
        "\ubc29\ubcc4 \uc800\uc7a5 \uaddc\uce59",
        "\uc218\ucde8\uc778\u00b7\uc774\uc57c\uae30\u00b7\uacb0\ub9d0 \uc5f0\ucd9c\uc548"
      ],
      "stageName": "\ud50c\ub808\uc774\uc5b4 \uc804\ud22c"
    },
    {
      "key": "ui",
      "path": "UI_\ud074\ub77c\uc774\uc5b8\ud2b8_\uc2dc\uc2a4\ud15c/06_UI_\ud074\ub77c\uc774\uc5b8\ud2b8_\uc2dc\uc2a4\ud15c.md",
      "aliases": [
        "UI",
        "\ud074\ub77c\uc774\uc5b8\ud2b8"
      ],
      "sections": [
        "\uac8c\uc784 \uac1c\uc694\uc640 \uc138\uacc4",
        "\ud0dd\ubc30\u00b7\uc0ac\ub9dd\u00b7\ubd80\ud65c",
        "\ubcf4\uc2a4 \uc804\ud22c\uc640 \uc2e4\ud328",
        "\ubcf4\uc2a4 \uaddc\uce59 \ubbf8\uacb0\uc815",
        "\uc5ed\ud560\uacfc \uacf5\ub3d9 \uacc4\uc57d",
        "\ub2e8\uacc4\ubcc4 \uac1c\ubc1c \ubc94\uc704",
        "\ub0a8\uc740 \uacb0\uc815",
        "\ubcf4\uc2a4 \ubc30\ub2ec \uc0c1\ud0dc",
        "\ubc30\ub2ec \uc120\ud0dd\uacfc \ubcf4\uc2a4 \uc9c4\ud589",
        "\ucf58\ud150\uce20 \ubaa9\ub85d\uacfc \ubc30\uc1a1\uc9c0 \uc694\uccad",
        "\ubc29\u00b7\uc800\uc7a5\u00b7\ud569\ub958",
        "\uc2dc\uac04 \uc81c\ud55c\uacfc \ubcf4\uc0c1",
        "\uc77c\ubc18 \ubc30\ub2ec \uc0c1\ud0dc",
        "\uacbd\uc81c\uc640 \ubc38\ub7f0\uc2a4",
        "\ucc28\ub7c9 \uc561\uc158\uacfc \uc131\uc7a5",
        "\ucc28\ub7c9 \uc0c1\ud638\uc791\uc6a9\u00b7\ud53c\ud574 \uaddc\uce59",
        "\ubc29\ubcc4 \uc800\uc7a5 \uaddc\uce59",
        "\ud654\uba74\uc5d0 \uc804\ub2ec\ud560 \uc0c1\ud0dc\uc640 \ubb38\uad6c",
        "\uc218\ucde8\uc778\u00b7\uc774\uc57c\uae30\u00b7\uacb0\ub9d0 \uc5f0\ucd9c\uc548"
      ],
      "stageName": "UI/\ud074\ub77c\uc774\uc5b8\ud2b8 \uc2dc\uc2a4\ud15c"
    }
  ],
  "tableSections": [
    "\ub2e8\uacc4\ubcc4 \uac1c\ubc1c \ubc94\uc704"
  ],
  "labels": {
    "navigation": "[\ub0b4 \ub2e8\uacc4\ubcc4 \uc791\uc5c5](#\ub2e8\uacc4\ubcc4-\uac1c\ubc1c-\ubc94\uc704) \u00b7 [\ud611\uc5c5 \uacc4\uc57d](#\uc5ed\ud560\uacfc-\uacf5\ub3d9-\uacc4\uc57d) \u00b7 [\ubcf4\uc2a4 \uacb0\uc815 \ub300\uae30](#\ubcf4\uc2a4-\uaddc\uce59-\ubbf8\uacb0\uc815) \u00b7 [\ub2f4\ub2f9 \uc778\uc218 \ud14c\uc2a4\ud2b8](#\ub2f4\ub2f9-\uc778\uc218-\ud14c\uc2a4\ud2b8) \u00b7 [\uad6c\ud604 \uae30\ub85d](#\uad6c\ud604\uacfc-\ub2f4\ub2f9-\uc0b0\ucd9c\ubb3c)",
    "note": "> \uc544\ub798\ub294 \ub2f4\ub2f9\uc5d0 \ud544\uc694\ud55c \uae30\ud68d \uae30\uc900\uc774\ub2e4. \uae30\ud68d \ubcc0\uacbd\uc740 \uae30\ud68d\uc790 \uc6d0\ubcf8\uc5d0 \ubc18\uc601\ud55c\ub2e4.",
    "testHeading": "## \ub2f4\ub2f9 \uc778\uc218 \ud14c\uc2a4\ud2b8",
    "defaultStage": "1\ub2e8\uacc4",
    "ownerPattern": "\ub2f4\ub2f9|\uc18c\uc720\uc790|\uc8fc\uad00|\ud655\uc778 \ud30c\ud2b8",
    "stepsPattern": "\uc2e4\ud589|\uc808\ucc28",
    "expectedPattern": "\uae30\ub300",
    "resultPattern": "\uacb0\uacfc",
    "sharedPattern": "\uc804\uccb4|\uacf5\ud1b5|\uc804 \ud30c\ud2b8"
  },
  "knownSections": [
    "\uac8c\uc784 \uac1c\uc694\uc640 \uc138\uacc4",
    "\ud0dd\ubc30\u00b7\uc0ac\ub9dd\u00b7\ubd80\ud65c",
    "\ubcf4\uc2a4 \uc804\ud22c\uc640 \uc2e4\ud328",
    "\ubcf4\uc2a4 \uaddc\uce59 \ubbf8\uacb0\uc815",
    "\uc5ed\ud560\uacfc \uacf5\ub3d9 \uacc4\uc57d",
    "\ub2e8\uacc4\ubcc4 \uac1c\ubc1c \ubc94\uc704",
    "\ub0a8\uc740 \uacb0\uc815",
    "\ubcf4\uc2a4 \ubc30\ub2ec \uc0c1\ud0dc",
    "\ubc30\ub2ec \uc120\ud0dd\uacfc \ubcf4\uc2a4 \uc9c4\ud589",
    "\ucf58\ud150\uce20 \ubaa9\ub85d\uacfc \ubc30\uc1a1\uc9c0 \uc694\uccad",
    "\ubc29\u00b7\uc800\uc7a5\u00b7\ud569\ub958",
    "\uc2dc\uac04 \uc81c\ud55c\uacfc \ubcf4\uc0c1",
    "\uc77c\ubc18 \ubc30\ub2ec \uc0c1\ud0dc",
    "\uacbd\uc81c\uc640 \ubc38\ub7f0\uc2a4",
    "\ucc28\ub7c9 \uc561\uc158\uacfc \uc131\uc7a5",
    "\ucc28\ub7c9 \uc0c1\ud638\uc791\uc6a9\u00b7\ud53c\ud574 \uaddc\uce59",
    "\uc77c\ubc18 \uc880\ube44 \ud0c0\uae43\u00b7\uac70\ub9ac \uad00\ub9ac",
    "\ubc29\ubcc4 \uc800\uc7a5 \uaddc\uce59",
    "\ud654\uba74\uc5d0 \uc804\ub2ec\ud560 \uc0c1\ud0dc\uc640 \ubb38\uad6c",
    "\uc218\ucde8\uc778\u00b7\uc774\uc57c\uae30\u00b7\uacb0\ub9d0 \uc5f0\ucd9c\uc548"
  ]
}
'@ | ConvertFrom-Json
$startMarker = '<!-- PLAN_SYNC_START -->'
$endMarker = '<!-- PLAN_SYNC_END -->'

function Read-Utf8 {
    param([string]$Path)
    return $utf8.GetString([IO.File]::ReadAllBytes($Path)).TrimStart([char]0xFEFF)
}

function Get-H2Sections {
    param([string]$Text)
    $sections = New-Object 'System.Collections.Generic.List[object]'
    $current = $null
    $fence = ''
    foreach ($line in ($Text -split '\r?\n')) {
        if ($line -match '^\s*(\x60{3,}|~{3,})') {
            $token = $Matches[1]
            if (-not $fence) { $fence = $token }
            elseif ($token[0] -eq $fence[0] -and $token.Length -ge $fence.Length) { $fence = '' }
        }
        if (-not $fence -and $line -match '^##\s+(.+?)\s*$') {
            $title = $Matches[1]
            if (@($sections | Where-Object { $_.Title -ceq $title }).Count) { throw "Duplicate source H2: $title" }
            $current = [pscustomobject]@{ Title = $title; Lines = (New-Object 'System.Collections.Generic.List[string]') }
            $sections.Add($current)
        }
        if ($null -ne $current) { $current.Lines.Add($line) }
    }
    return $sections.ToArray()
}

function Split-TableCells {
    param([string]$Line)
    return @([regex]::Split($Line.Trim().Trim('|'), '(?<!\\)\|') | ForEach-Object { $_.Trim() })
}

function Filter-StageTables {
    param([string]$Text, [string]$StageName)
    $lines = $Text -split '\r?\n'
    $result = New-Object 'System.Collections.Generic.List[string]'
    for ($i = 0; $i -lt $lines.Count; $i++) {
        if ($lines[$i] -match '^\s*\|' -and $i + 1 -lt $lines.Count -and $lines[$i + 1] -match '^\s*\|?[\s:|-]+\|\s*$') {
            $result.Add($lines[$i])
            $result.Add($lines[++$i])
            $selected = 0
            while ($i + 1 -lt $lines.Count -and $lines[$i + 1] -match '^\s*\|') {
                $i++
                $cells = @(Split-TableCells $lines[$i])
                if ($cells[0] -ceq $StageName) { $result.Add($lines[$i]); $selected++ }
            }
            if ($selected -ne 1) { throw "Expected one stage row for '$StageName'; found $selected." }
        } else { $result.Add($lines[$i]) }
    }
    return $result -join $lf
}

function Get-HeadingAnchors {
    param([string]$Text)
    $anchors = New-Object 'System.Collections.Generic.HashSet[string]'
    $fence = ''
    foreach ($line in ($Text -split '\r?\n')) {
        if ($line -match '^\s*(\x60{3,}|~{3,})') {
            $token = $Matches[1]
            if (-not $fence) { $fence = $token }
            elseif ($token[0] -eq $fence[0] -and $token.Length -ge $fence.Length) { $fence = '' }
        }
        if (-not $fence -and $line -match '^#{1,6}\s+(.+?)\s*$') {
            $slug = $Matches[1].ToLowerInvariant() -replace '[^\p{L}\p{N}\s-]', '' -replace '\s+', '-'
            [void]$anchors.Add($slug)
        }
    }
    return ,$anchors
}

function Convert-RelativeLinks {
    param([string]$Text, [string]$SourcePath, [string]$TargetPath, [System.Collections.Generic.HashSet[string]]$SelectedAnchors)
    $baseUri = New-Object Uri($SourcePath)
    $targetUri = New-Object Uri([IO.Path]::GetDirectoryName($TargetPath) + [IO.Path]::DirectorySeparatorChar)
    $result = New-Object 'System.Collections.Generic.List[string]'
    $fence = ''
    foreach ($line in ($Text -split '\r?\n')) {
        if ($line -match '^\s*(\x60{3,}|~{3,})') {
            $token = $Matches[1]
            if (-not $fence) { $fence = $token }
            elseif ($token[0] -eq $fence[0] -and $token.Length -ge $fence.Length) { $fence = '' }
            $result.Add($line)
            continue
        }
        if ($fence) { $result.Add($line); continue }
        $rewritten = [regex]::Replace($line, '(?<code>\x60+[^\x60]*\x60+)|(?<link>!?\[[^\]]*\]\((?<dest><[^>]+>|[^)\r\n]+)\))', {
            param($match)
            if ($match.Groups['code'].Success) { return $match.Value }
            $cleaned = $match.Groups['dest'].Value.Trim()
            $title = ''
            if ($cleaned -match '^(.*?)(\s+["''][^"'']*["''])$') { $cleaned = $Matches[1]; $title = $Matches[2] }
            $angled = $cleaned.StartsWith('<') -and $cleaned.EndsWith('>')
            if ($angled) { $cleaned = $cleaned.Substring(1, $cleaned.Length - 2) }
            if ($cleaned -match '^[a-zA-Z][a-zA-Z0-9+.-]*:' -or $cleaned.StartsWith('//')) { return $match.Value }
            if ($cleaned.StartsWith('#')) {
                if ($SelectedAnchors.Contains([Uri]::UnescapeDataString($cleaned.Substring(1)))) { return $match.Value }
                $relative = [Uri]::UnescapeDataString($targetUri.MakeRelativeUri($baseUri).ToString()) + $cleaned
            } else {
                $resolved = New-Object Uri($baseUri, $cleaned)
                if (-not $resolved.IsFile) { return $match.Value }
                $relative = [Uri]::UnescapeDataString($targetUri.MakeRelativeUri($resolved).ToString())
            }
            if ($angled -or $relative -match '\s') { $relative = '<' + $relative + '>' }
            return $match.Value.Substring(0, $match.Groups['dest'].Index - $match.Index) + $relative + $title + ')'
        })
        $result.Add($rewritten)
    }
    return $result -join $lf
}

function Test-RoleOwner {
    param([string]$Owner, [object]$Role)
    if ($Owner -match $config.labels.sharedPattern) { return $true }
    foreach ($alias in $Role.aliases) {
        if ($Owner.IndexOf($alias, [StringComparison]::OrdinalIgnoreCase) -ge 0) { return $true }
    }
    return $false
}

function Get-RoleTests {
    param([string]$Text, [object]$Role)
    $lines = $Text -split '\r?\n'
    $stage = $config.labels.defaultStage
    $groups = New-Object 'System.Collections.Generic.List[object]'
    $ids = New-Object 'System.Collections.Generic.HashSet[string]'
    $fence = ''
    for ($i = 0; $i -lt $lines.Count; $i++) {
        $line = $lines[$i]
        if ($line -match '^\s*(\x60{3,}|~{3,})') {
            $token = $Matches[1]
            if (-not $fence) { $fence = $token }
            elseif ($token[0] -eq $fence[0] -and $token.Length -ge $fence.Length) { $fence = '' }
            continue
        }
        if ($fence) { continue }
        if ($line -match '^#{1,3}\s+(.+)$') {
            $heading = $Matches[1]
            if ($heading -match '[123]\s*\uB2E8\uACC4') { $stage = $heading }
        }
        if ($line -notmatch '^\s*\|' -or $i + 1 -ge $lines.Count -or $lines[$i + 1] -notmatch '^\s*\|?[\s:|-]+\|\s*$') { continue }
        $headers = @(Split-TableCells $line)
        $idIndex = -1; $ownerIndex = -1; $stepsIndex = -1; $expectedIndex = -1; $resultIndex = -1; $playersIndex = -1
        for ($j = 0; $j -lt $headers.Count; $j++) {
            if ($headers[$j] -ceq 'ID') { $idIndex = $j }
            elseif ($headers[$j] -match $config.labels.ownerPattern) { $ownerIndex = $j }
            elseif ($headers[$j] -match $config.labels.stepsPattern) { $stepsIndex = $j }
            elseif ($headers[$j] -match $config.labels.expectedPattern) { $expectedIndex = $j }
            elseif ($headers[$j] -match $config.labels.resultPattern) { $resultIndex = $j }
            elseif ($headers[$j] -match '\uC778\uC6D0') { $playersIndex = $j }
        }
        if ($idIndex -lt 0 -or $stepsIndex -lt 0 -or $expectedIndex -lt 0) { continue }
        if ($ownerIndex -lt 0 -or $resultIndex -lt 0) { throw "Test table needs owner and result columns: $stage" }
        $columns = @($idIndex)
        if ($playersIndex -ge 0) { $columns += $playersIndex }
        $columns += @($stepsIndex, $expectedIndex, $ownerIndex, $resultIndex)
        $rows = New-Object 'System.Collections.Generic.List[string]'
        $i++
        while ($i + 1 -lt $lines.Count -and $lines[$i + 1] -match '^\s*\|') {
            $i++
            $cells = @(Split-TableCells $lines[$i])
            if ($cells.Count -ne $headers.Count) { throw "Invalid test table cell count: $($cells[0])" }
            if (-not $ids.Add($cells[$idIndex])) { throw "Duplicate test ID: $($cells[$idIndex])" }
            if (Test-RoleOwner $cells[$ownerIndex] $Role) { $rows.Add('| ' + (($columns | ForEach-Object { $cells[$_] }) -join ' | ') + ' |') }
        }
        if ($rows.Count) {
            $groups.Add([pscustomobject]@{
                Stage = $stage
                Header = ('| ' + (($columns | ForEach-Object { $headers[$_] }) -join ' | ') + ' |')
                Separator = ('| ' + (($columns | ForEach-Object { '---' }) -join ' | ') + ' |')
                Rows = $rows.ToArray()
            })
        }
    }
    if (-not $groups.Count) { throw "No assigned test rows found for $($Role.key)." }
    $output = New-Object 'System.Collections.Generic.List[string]'
    $output.Add($config.labels.testHeading)
    foreach ($group in $groups) {
        $output.Add('')
        $output.Add('### ' + $group.Stage)
        $output.Add('')
        $output.Add($group.Header)
        $output.Add($group.Separator)
        foreach ($row in $group.Rows) { $output.Add($row) }
    }
    return $output -join $lf
}

function Get-UpdatedBytes {
    param([byte[]]$Bytes, [string]$GeneratedText, [string]$Path)
    $text = $utf8.GetString($Bytes)
    $starts = [regex]::Matches($text, [regex]::Escape($startMarker))
    $ends = [regex]::Matches($text, [regex]::Escape($endMarker))
    if ($starts.Count -ne 1 -or $ends.Count -ne 1 -or $starts[0].Index -ge $ends[0].Index) { throw "Expected exactly one ordered sync marker pair: $Path" }
    $prefixBytes = $utf8.GetByteCount($text.Substring(0, $starts[0].Index + $startMarker.Length))
    $suffixBytes = $utf8.GetByteCount($text.Substring(0, $ends[0].Index))
    $newline = if ($text.Contains($crlf)) { $crlf } else { $lf }
    $middle = $newline + (($GeneratedText -replace '\r?\n', $newline).TrimEnd([char[]]($crlf))) + $newline
    $middleBytes = $utf8.GetBytes($middle)
    $stream = New-Object IO.MemoryStream
    try {
        $stream.Write($Bytes, 0, $prefixBytes)
        $stream.Write($middleBytes, 0, $middleBytes.Length)
        $stream.Write($Bytes, $suffixBytes, $Bytes.Length - $suffixBytes)
        return ,$stream.ToArray()
    } finally { $stream.Dispose() }
}

function Test-BytesEqual {
    param([byte[]]$Left, [byte[]]$Right)
    if ($Left.Length -ne $Right.Length) { return $false }
    for ($i = 0; $i -lt $Left.Length; $i++) { if ($Left[$i] -ne $Right[$i]) { return $false } }
    return $true
}

if (-not $PlanRoot) { $PlanRoot = Split-Path -Parent $PSCommandPath }
$PlanRoot = [IO.Path]::GetFullPath($PlanRoot)
$sourcePath = Join-Path $PlanRoot $config.source
$testPath = Join-Path $PlanRoot $config.tests
$sections = @(Get-H2Sections (Read-Utf8 $sourcePath))
$testsText = Read-Utf8 $testPath
foreach ($title in $config.knownSections) {
    if (-not @($sections | Where-Object { $_.Title -ceq $title }).Count) { throw "Missing source H2: $title" }
}
foreach ($section in $sections) {
    if ($config.knownSections -cnotcontains $section.Title) { throw "Unmapped source H2: $($section.Title)" }
}
$updates = New-Object 'System.Collections.Generic.List[object]'
foreach ($role in $config.roles) {
    $targetPath = Join-Path $PlanRoot $role.path
    $selected = @($sections | Where-Object { $role.sections -ccontains $_.Title -or $_.Title -ceq $config.bossHeading })
    $blocks = @($selected | ForEach-Object {
        $body = ($_.Lines.ToArray() -join $lf).TrimEnd([char[]]($crlf))
        if ($config.tableSections -ccontains $_.Title) { $body = Filter-StageTables $body $role.stageName }
        $body
    })
    $planningText = $blocks -join ($lf + $lf)
    $planningText = Convert-RelativeLinks $planningText $sourcePath $targetPath (Get-HeadingAnchors $planningText)
    $roleTests = Get-RoleTests $testsText $role
    $roleTests = Convert-RelativeLinks $roleTests $testPath $targetPath (Get-HeadingAnchors $roleTests)
    $generated = $config.labels.note + $lf + $lf + $config.labels.navigation + $lf + $lf + $planningText + $lf + $lf + $roleTests
    $before = [IO.File]::ReadAllBytes($targetPath)
    $after = Get-UpdatedBytes $before $generated $targetPath
    $updates.Add([pscustomobject]@{ Path = $targetPath; Before = $before; After = $after; Changed = -not (Test-BytesEqual $before $after) })
}

# All five targets are validated before any file is written.
$changed = @($updates | Where-Object Changed)
if ($Check) {
    if ($changed.Count) { throw ("Personal docs need sync: " + (($changed | ForEach-Object { $_.Path }) -join ', ')) }
    Write-Output 'All 5 personal documents are current.'
    return
}
foreach ($update in $updates) {
    if (-not (Test-BytesEqual ([IO.File]::ReadAllBytes($update.Path)) $update.Before)) { throw "A personal document changed during sync; retry after editing: $($update.Path)" }
}
foreach ($update in $changed) {
    [IO.File]::WriteAllBytes($update.Path, $update.After)
    Write-Output ('Updated: ' + $update.Path)
}
Write-Output ('Sync complete: {0} changed, {1} unchanged.' -f $changed.Count, ($updates.Count - $changed.Count))
