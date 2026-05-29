# perf-compare.ps1 - Timing + diff harness for mType benchmarks.
#
# Modes:
#   baseline  - Run N iterations, save JSON snapshot under .perf-baselines/.
#   compare   - Run N iterations, compare to the saved baseline, print delta + verdict.
#   bisect    - Run N iterations, exit 1 if median regressed >= -RegressionPct vs baseline.
#               Suitable for `git bisect run`.
#   diff      - Run once with -ConfigA and once with -ConfigB, diff stdout.
#               Use to check JIT-on vs --no-jit correctness on demand.
#   suite     - Apply baseline/compare across a glob of .mt files; aggregate verdicts.
#
# Convention: scripts in .claude/skills/runtime-perf/scripts/. Invoke from repo root.

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('baseline', 'compare', 'bisect', 'diff', 'suite')]
    [string]$Mode,

    [string]$Benchmark,
    [string]$Suite,
    [int]$Iterations = 7,
    [string]$Exe = 'bin\mType\Release\x64\mType.exe',
    [string]$ConfigA = '',
    [string]$ConfigB = '--no-jit',
    [string]$BaselineDir = '.perf-baselines',
    [double]$RegressionPct = 3.0,
    [string]$Label = ''
)

$ErrorActionPreference = 'Stop'

function Assert-Path($p, $what) {
    if (-not (Test-Path -LiteralPath $p)) { throw "$what not found: $p" }
}

function Get-BenchmarkKey([string]$bench, [string]$cfg) {
    $name = [IO.Path]::GetFileNameWithoutExtension($bench)
    $cfgKey = if ([string]::IsNullOrWhiteSpace($cfg)) { 'jit' } else { ($cfg -replace '[^A-Za-z0-9]+', '_').Trim('_') }
    "$name.$cfgKey"
}

function Get-BaselinePath([string]$bench, [string]$cfg) {
    if (-not (Test-Path -LiteralPath $BaselineDir)) {
        New-Item -ItemType Directory -Path $BaselineDir | Out-Null
    }
    Join-Path $BaselineDir ((Get-BenchmarkKey $bench $cfg) + '.json')
}

function Invoke-Benchmark([string]$bench, [string]$cfg, [int]$iter) {
    Assert-Path $Exe 'mType.exe (Release build required)'
    Assert-Path $bench 'Benchmark .mt'
    $extraArgs = @()
    if (-not [string]::IsNullOrWhiteSpace($cfg)) { $extraArgs += ($cfg -split '\s+') }
    $rawArgs = @("--benchmark=$bench", "--benchmark-iterations=$iter", '--benchmark-output=json') + $extraArgs
    $stdout = & $Exe @rawArgs 2>&1
    $text = ($stdout -join "`n").Trim()
    $jsonStart = $text.IndexOf('{')
    if ($jsonStart -lt 0) { throw "No JSON in output:`n$text" }
    return ($text.Substring($jsonStart) | ConvertFrom-Json)
}

function Get-Stats($result) {
    $script = $result.scripts[0]
    if (-not $script.ok) { throw "Benchmark failed: $($script.name)" }
    [pscustomobject]@{
        name         = $script.name
        iterations   = $script.iterations
        median_ms    = [double]$script.wall_ms.median
        mean_ms      = [double]$script.wall_ms.mean
        stddev_ms    = [double]$script.wall_ms.stddev
        min_ms       = [double]$script.wall_ms.min
        exec_ms      = [double]$script.exec_ms
        instructions = [int64]$script.instructions
    }
}

function Format-Stats($s, [string]$tag) {
    "  {0,-10} median={1,8:F2}ms mean={2,8:F2}ms stddev={3,7:F2}ms exec={4,8:F2}ms ins={5}" `
        -f $tag, $s.median_ms, $s.mean_ms, $s.stddev_ms, $s.exec_ms, $s.instructions
}

function Get-Verdict($baseline, $candidate, [double]$thresholdPct) {
    $deltaMs = $candidate.median_ms - $baseline.median_ms
    $deltaPct = if ($baseline.median_ms -gt 0) { 100.0 * $deltaMs / $baseline.median_ms } else { 0.0 }
    $pooled = [Math]::Sqrt(($baseline.stddev_ms * $baseline.stddev_ms) + ($candidate.stddev_ms * $candidate.stddev_ms))
    $significant = ($pooled -gt 0) -and ([Math]::Abs($deltaMs) / $pooled -gt 2.0)

    $verdict = 'NOISE'
    if ($significant -and $deltaPct -le -$thresholdPct) { $verdict = 'IMPROVEMENT' }
    elseif ($significant -and $deltaPct -ge $thresholdPct) { $verdict = 'REGRESSION' }

    [pscustomobject]@{
        delta_ms    = [Math]::Round($deltaMs, 2)
        delta_pct   = [Math]::Round($deltaPct, 2)
        significant = $significant
        verdict     = $verdict
    }
}

function Save-Snapshot([string]$bench, [string]$cfg, $stats) {
    $path = Get-BaselinePath $bench $cfg
    $payload = [pscustomobject]@{
        captured_utc = (Get-Date).ToUniversalTime().ToString('o')
        config       = $cfg
        label        = $Label
        stats        = $stats
    }
    $payload | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $path -Encoding utf8
    Write-Host "saved baseline -> $path"
}

function Load-Snapshot([string]$bench, [string]$cfg) {
    $path = Get-BaselinePath $bench $cfg
    Assert-Path $path 'baseline snapshot (run -Mode baseline first)'
    return (Get-Content -LiteralPath $path -Raw | ConvertFrom-Json)
}

function Invoke-One([string]$bench) {
    switch ($Mode) {
        'baseline' {
            $cfg = $ConfigA
            $stats = Get-Stats (Invoke-Benchmark $bench $cfg $Iterations)
            Write-Host ("[{0}] {1}" -f (Get-BenchmarkKey $bench $cfg), $bench)
            Write-Host (Format-Stats $stats 'baseline')
            Save-Snapshot $bench $cfg $stats
        }
        'compare' {
            $cfg = $ConfigA
            $base = Load-Snapshot $bench $cfg
            $cand = Get-Stats (Invoke-Benchmark $bench $cfg $Iterations)
            $v = Get-Verdict $base.stats $cand $RegressionPct
            Write-Host ("[{0}] {1}" -f (Get-BenchmarkKey $bench $cfg), $bench)
            Write-Host (Format-Stats $base.stats 'baseline')
            Write-Host (Format-Stats $cand 'candidate')
            Write-Host ("  verdict   {0}  delta={1:F2}ms ({2:F2}%) significant={3}" -f $v.verdict, $v.delta_ms, $v.delta_pct, $v.significant)
            return $v
        }
        'bisect' {
            $cfg = $ConfigA
            $base = Load-Snapshot $bench $cfg
            $cand = Get-Stats (Invoke-Benchmark $bench $cfg $Iterations)
            $v = Get-Verdict $base.stats $cand $RegressionPct
            Write-Host ("[bisect] {0} verdict={1} delta_pct={2:F2}" -f $bench, $v.verdict, $v.delta_pct)
            if ($v.verdict -eq 'REGRESSION') { exit 1 } else { exit 0 }
        }
        'diff' {
            Assert-Path $Exe 'mType.exe'
            $argsA = @($bench); if ($ConfigA) { $argsA = ($ConfigA -split '\s+') + $argsA }
            $argsB = @($bench); if ($ConfigB) { $argsB = ($ConfigB -split '\s+') + $argsB }
            $outA = (& $Exe @argsA) -join "`n"
            $outB = (& $Exe @argsB) -join "`n"
            if ($outA -eq $outB) {
                Write-Host "[diff] IDENTICAL ($ConfigA vs $ConfigB)"
            } else {
                Write-Host "[diff] DIVERGED ($ConfigA vs $ConfigB)"
                $tmpA = New-TemporaryFile; $tmpB = New-TemporaryFile
                Set-Content -LiteralPath $tmpA -Value $outA -Encoding utf8
                Set-Content -LiteralPath $tmpB -Value $outB -Encoding utf8
                Compare-Object (Get-Content $tmpA) (Get-Content $tmpB) | Format-Table -AutoSize | Out-Host
                Remove-Item $tmpA, $tmpB -Force
                exit 1
            }
        }
    }
}

if ($Mode -eq 'suite') {
    if (-not $Suite) { throw '-Suite <glob> required for suite mode' }
    $files = Get-ChildItem -Path $Suite -File -ErrorAction Stop
    if (-not $files) { throw "No files matched $Suite" }
    $regressions = @()
    foreach ($f in $files) {
        $Benchmark = $f.FullName
        try {
            # Suite mode delegates to baseline or compare based on whether snapshot exists.
            $cfg = $ConfigA
            $snapPath = Get-BaselinePath $Benchmark $cfg
            $Mode = if (Test-Path -LiteralPath $snapPath) { 'compare' } else { 'baseline' }
            $v = Invoke-One $Benchmark
            if ($v -and $v.verdict -eq 'REGRESSION') { $regressions += $f.Name }
        } catch {
            Write-Warning "[$($f.Name)] $_"
        }
    }
    if ($regressions.Count -gt 0) {
        Write-Host ""
        Write-Host "REGRESSIONS:"
        $regressions | ForEach-Object { Write-Host "  $_" }
        exit 1
    }
    Write-Host ""
    Write-Host "no regressions across $($files.Count) benchmarks"
    exit 0
}

if (-not $Benchmark) { throw "-Benchmark <path.mt> required for mode '$Mode'" }
Invoke-One $Benchmark | Out-Null
