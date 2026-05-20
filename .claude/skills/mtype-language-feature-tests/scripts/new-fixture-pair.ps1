# new-fixture-pair.ps1
# Scaffolds a .mt + .expected pair under mType/tests/testFiles/<feature>/<variant>/.
#
# Usage (from repo root, Windows PowerShell 5.1 or PowerShell 7):
#   powershell -ExecutionPolicy Bypass -File .claude/skills/mtype-language-feature-tests/scripts/new-fixture-pair.ps1 `
#       -Feature cast -Variant pass -Name castNestedGenericTriple
#
#   powershell -ExecutionPolicy Bypass -File .claude/skills/mtype-language-feature-tests/scripts/new-fixture-pair.ps1 `
#       -Feature cast -Variant error -Name castNullToNonNullable_error
#
# Behavior:
#   - pass:  creates <Name>.mt and <Name>.expected (skeleton with "// Expected output:" block)
#   - error: creates <Name>.mt only (ERROR_EXPECTED tests have no .expected file)
#   - Refuses to overwrite existing files.
#   - Prints the suite-registration line to copy into <Feature>TestSuite.cpp.

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Feature,
    [Parameter(Mandatory = $true)][ValidateSet('pass', 'error')][string]$Variant,
    [Parameter(Mandatory = $true)][string]$Name,
    [string]$Root = (Get-Location).Path
)

$ErrorActionPreference = 'Stop'

$dir = Join-Path $Root "mType/tests/testFiles/$Feature/$Variant"
if (-not (Test-Path $dir)) {
    Write-Error "Fixture directory does not exist: $dir`nCreate it first, or check the feature name against the suite map in REFERENCE.md."
    exit 2
}

$mtPath = Join-Path $dir "$Name.mt"
$expPath = Join-Path $dir "$Name.expected"

if (Test-Path $mtPath) {
    Write-Error "Already exists: $mtPath"
    exit 2
}

if ($Variant -eq 'pass') {
    if (Test-Path $expPath) {
        Write-Error "Already exists: $expPath"
        exit 2
    }

    $mtSkeleton = @"
// Test: <one-line description of behavior>

// TODO: implement the test body. End with print(...) lines whose output
// matches the sibling .expected file exactly.

// Expected output:
// <line 1>
"@
    Set-Content -Path $mtPath -Value $mtSkeleton -Encoding utf8

    $expSkeleton = "<line 1>`n"
    Set-Content -Path $expPath -Value $expSkeleton -Encoding utf8 -NoNewline

    Write-Host "Created: $mtPath"
    Write-Host "Created: $expPath"
    Write-Host ""
    Write-Host "Register in <Feature>TestSuite.cpp:"
    Write-Host "    addOutputVerificationTest(`"<Human Readable Name>`","
    Write-Host "                    passPath + `"$Name.mt`");"
}
else {
    $mtSkeleton = @"
// Error: <one-line description of what must fail and why>

// TODO: implement the failing test body.
"@
    Set-Content -Path $mtPath -Value $mtSkeleton -Encoding utf8

    Write-Host "Created: $mtPath"
    Write-Host ""
    Write-Host "Register in <Feature>TestSuite.cpp:"
    Write-Host "    addTestFromFile(`"<Human Readable Name>`","
    Write-Host "                    errorPath + `"$Name.mt`","
    Write-Host "                    TestType::ERROR_EXPECTED);"
    Write-Host ""
    Write-Host "Or to pin the error message (MYT-38):"
    Write-Host "    addTestFromFile(`"<Human Readable Name>`","
    Write-Host "                    errorPath + `"$Name.mt`","
    Write-Host "                    TestType::ERROR_EXPECTED,"
    Write-Host "                    `"MT-EXXXX`");"
}

exit 0
