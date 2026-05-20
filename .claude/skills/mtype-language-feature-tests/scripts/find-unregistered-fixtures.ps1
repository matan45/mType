# find-unregistered-fixtures.ps1
# Flags .mt fixtures under mType/tests/testFiles/ that no TestSuite .cpp registers.
#
# Usage (from repo root):
#   powershell -ExecutionPolicy Bypass -File .claude/skills/mtype-language-feature-tests/scripts/find-unregistered-fixtures.ps1
#   powershell -ExecutionPolicy Bypass -File .claude/skills/mtype-language-feature-tests/scripts/find-unregistered-fixtures.ps1 -Feature cast
#
# Exit code 0 = every .mt fixture is registered. 1 = at least one orphan.
#
# Detection: a .mt file is "registered" if its bare filename (e.g. "castToInterface.mt")
# appears as a string literal in any mType/tests/suites/*.cpp. Conservative — false
# positives are unlikely because filenames are unique enough.
#
# What this catches:
#   - You scaffolded a fixture and forgot to add addOutputVerificationTest(...).
#   - You renamed a suite registration but not the .mt file (or vice versa).
#
# What this does NOT catch:
#   - .expected without a sibling .mt (or vice versa) — different check.
#   - Tests registered but using the wrong test type.

[CmdletBinding()]
param(
    [string]$Feature = '',
    [string]$Root = (Get-Location).Path
)

$ErrorActionPreference = 'Stop'

$fixturesRoot = if ($Feature) {
    Join-Path $Root "mType/tests/testFiles/$Feature"
} else {
    Join-Path $Root 'mType/tests/testFiles'
}

if (-not (Test-Path $fixturesRoot)) {
    Write-Error "Path not found: $fixturesRoot"
    exit 2
}

$suitesDir = Join-Path $Root 'mType/tests/suites'
if (-not (Test-Path $suitesDir)) {
    Write-Error "Path not found: $suitesDir"
    exit 2
}

# Collect every string literal that looks like an .mt filename across all suite .cpp files.
$registeredNames = @{}
$suiteFiles = Get-ChildItem -Path $suitesDir -Filter '*.cpp' -Recurse
foreach ($file in $suiteFiles) {
    $content = Get-Content -Raw -Path $file.FullName
    # Match any "...something.mt" inside double quotes. Lazy so each literal matches once.
    $literalMatches = [regex]::Matches($content, '"([^"]*?\.mt)"')
    foreach ($m in $literalMatches) {
        $literal = $m.Groups[1].Value
        # Reduce to bare filename for lookup.
        $bare = Split-Path -Leaf $literal
        $registeredNames[$bare] = $true
    }
}

# Walk fixtures and report orphans.
$mtFiles = Get-ChildItem -Path $fixturesRoot -Filter '*.mt' -Recurse
$orphans = @()
foreach ($mt in $mtFiles) {
    if (-not $registeredNames.ContainsKey($mt.Name)) {
        # Use repo-relative path for readability.
        $rel = $mt.FullName.Substring($Root.Length).TrimStart('\', '/')
        $orphans += $rel
    }
}

if ($orphans.Count -eq 0) {
    if ($Feature) {
        Write-Host "OK: every .mt fixture under mType/tests/testFiles/$Feature/ is registered."
    } else {
        Write-Host "OK: every .mt fixture under mType/tests/testFiles/ is registered."
    }
    exit 0
}

Write-Host "Unregistered .mt fixtures ($($orphans.Count)):"
foreach ($o in ($orphans | Sort-Object)) {
    Write-Host "  $o"
}
Write-Host ""
Write-Host "Each fixture above exists but no TestSuite .cpp references its filename."
Write-Host "Either register it in the owning suite, or delete the fixture if it's dead."
exit 1
