$exe = "C:\matan\mType\bin\mType\Debug\x64\mType.exe"
$base = "C:\matan\mType\mType\tests\testFiles\class\pass"
$tests = @("jitBenchmark", "jitFloatArithmetic", "jitObjectOps", "jitArrayOps")

foreach ($t in $tests) {
    $file = "$base\$t.mt"

    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    & $exe $file 2>&1 | Out-Null
    $sw.Stop()
    $jitMs = $sw.ElapsedMilliseconds

    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    & $exe --no-jit $file 2>&1 | Out-Null
    $sw.Stop()
    $noJitMs = $sw.ElapsedMilliseconds

    if ($noJitMs -gt 0) {
        $speedup = [math]::Round($noJitMs / $jitMs, 2)
    } else {
        $speedup = "N/A"
    }

    Write-Host ("{0,-25} JIT: {1,6} ms   no-JIT: {2,6} ms   speedup: {3}x" -f $t, $jitMs, $noJitMs, $speedup)
}
