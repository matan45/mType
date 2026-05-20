# check-file-sizes.ps1
# Flags source files that exceed the project's hard limits.
#
# Usage (from repo root, Windows PowerShell 5.1 or PowerShell 7):
#   powershell -ExecutionPolicy Bypass -File .claude/skills/refactor/scripts/check-file-sizes.ps1
#   powershell -ExecutionPolicy Bypass -File .claude/skills/refactor/scripts/check-file-sizes.ps1 -CppLimit 500 -TsLimit 500
#
# Exit code 0 = all files within limits, 1 = at least one hard violation.

[CmdletBinding()]
param(
    [int]$CppLimit = 500,
    [int]$TsLimit = 500,
    [string]$Root = (Get-Location).Path
)

# Deep-module exceptions documented in CLAUDE.md. Update both together.
$DeepModuleExceptions = @(
    'mType/parser/ClassParser.cpp',
    'mType/parser/StatementParser.cpp',
    'mType/parser/ExpressionParser.cpp',
    'mType/parser/InterfaceParser.cpp',
    'mType/parser/TypeParser.cpp',
    'mType/parser/utilities/StatementTypeDetector.cpp'
) | ForEach-Object { $_.Replace('/', [System.IO.Path]::DirectorySeparatorChar) }

# Directories scanned but excluded from the size limit altogether.
# Test suites are data-heavy registration code, not behavior logic.
$ExcludedDirs = @(
    'mType/tests',
    'languageserver/tests'
) | ForEach-Object { $_.Replace('/', [System.IO.Path]::DirectorySeparatorChar) }

# Scan roots.
$CppRoots = @(
    'mType',
    'languageserver/src',
    'languageserver/tests',
    'packagemanager/src'
)
$TsRoots  = @('mtype-vscode-extension/src')

# Extensions scanned per language.
$CppExtensions = @('.cpp', '.hpp')
$TsExtensions  = @('.ts')

function Get-RelativePath {
    param([string]$Full, [string]$Base)
    $sep = [System.IO.Path]::DirectorySeparatorChar
    $baseNorm = $Base.TrimEnd($sep, '/') + $sep
    if ($Full.StartsWith($baseNorm, [System.StringComparison]::OrdinalIgnoreCase)) {
        return $Full.Substring($baseNorm.Length)
    }
    return $Full
}

function Test-Excluded {
    param([string]$RelPath)
    foreach ($ex in $DeepModuleExceptions) {
        if ($RelPath -eq $ex) { return $true }
    }
    return $false
}

function Test-InExcludedDir {
    param([string]$RelPath)
    $sep = [System.IO.Path]::DirectorySeparatorChar
    foreach ($d in $ExcludedDirs) {
        $prefix = $d.TrimEnd($sep) + $sep
        if ($RelPath.StartsWith($prefix, [System.StringComparison]::OrdinalIgnoreCase)) {
            return $true
        }
    }
    return $false
}

function Scan-Files {
    param(
        [string[]]$Roots,
        [string[]]$Extensions,
        [int]$Limit,
        [string]$Label
    )
    $violations = @()
    foreach ($r in $Roots) {
        $full = Join-Path $Root $r
        if (-not (Test-Path $full)) { continue }
        foreach ($ext in $Extensions) {
            Get-ChildItem -Path $full -Recurse -Filter "*$ext" -File | ForEach-Object {
                $rel = Get-RelativePath -Full $_.FullName -Base $Root
                if (Test-InExcludedDir -RelPath $rel) { return }
                $lineCount = (Get-Content -Path $_.FullName | Measure-Object -Line).Lines
                if ($lineCount -gt $Limit) {
                    $excluded = Test-Excluded -RelPath $rel
                    $violations += [pscustomobject]@{
                        Label    = $Label
                        Path     = $rel
                        Lines    = $lineCount
                        Limit    = $Limit
                        Excepted = $excluded
                    }
                }
            }
        }
    }
    return $violations
}

$cppViolations = Scan-Files -Roots $CppRoots -Extensions $CppExtensions -Limit $CppLimit -Label 'C++'
$tsViolations  = Scan-Files -Roots $TsRoots  -Extensions $TsExtensions  -Limit $TsLimit  -Label 'TypeScript'

$all = @($cppViolations) + @($tsViolations) | Sort-Object -Property Lines -Descending

if ($all.Count -eq 0) {
    Write-Output "OK: no files exceed limits (cpp=$CppLimit, ts=$TsLimit)."
    exit 0
}

$hardViolations = $all | Where-Object { -not $_.Excepted }
$noted          = $all | Where-Object { $_.Excepted }

if ($hardViolations.Count -gt 0) {
    Write-Output ""
    Write-Output "Hard violations (must refactor or document as exception):"
    Write-Output "---------------------------------------------------------"
    $hardViolations | ForEach-Object {
        '{0,6} lines  [{1}]  {2}' -f $_.Lines, $_.Label, $_.Path
    } | Write-Output
}

if ($noted.Count -gt 0) {
    Write-Output ""
    Write-Output "Documented exceptions (CLAUDE.md deep-module list):"
    Write-Output "---------------------------------------------------"
    $noted | ForEach-Object {
        '{0,6} lines  [{1}]  {2}' -f $_.Lines, $_.Label, $_.Path
    } | Write-Output
}

Write-Output ""
Write-Output ("Summary: {0} hard violation(s), {1} documented exception(s)." -f $hardViolations.Count, $noted.Count)

if ($hardViolations.Count -gt 0) { exit 1 } else { exit 0 }
