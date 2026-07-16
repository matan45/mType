# Release Build Optimization Experiments

The root Premake file exposes one opt-in option for Release-build experiments:

```text
--release-optimization=lto
--release-optimization=pgo-instrument
--release-optimization=pgo-optimize
```

The option takes exactly one value, so the modes cannot be combined. Omitting
it preserves the ordinary build settings. All three modes enable whole-program
optimization; the two PGO modes additionally configure the final `mType.exe`
link. PGO is deliberately limited to Visual Studio/MSVC generation and fails
early for another Premake action.

Run the Windows commands below from a Visual Studio Developer PowerShell at the
repository root. Invoke `premake5` directly: `runPremake.bat` intentionally
generates the default configuration and does not forward experiment options.

## Default baseline

Regenerate and rebuild whenever switching from an experiment. This prevents
object files compiled with `/GL` from contaminating the default baseline.

```powershell
premake5 vs2022
msbuild Interpreter.sln /m /t:mType:Rebuild /p:Configuration=Release /p:Platform=x64

bin\mType\Release\x64\mType.exe --tests
bin\mType\Release\x64\mType.exe --tests --no-jit
bin\mType\Release\x64\mType.exe --benchmark --benchmark-iterations=7 --benchmark-output=json --jit-stats > perf-default-jit.jsonl
bin\mType\Release\x64\mType.exe --benchmark --benchmark-iterations=7 --benchmark-output=json --no-jit > perf-default-nojit.jsonl
```

## LTO experiment

Premake's `linktimeoptimization "On"` setting supplies the toolset-specific
compile and link settings (`/GL` and `/LTCG` for MSVC) to every Release module.

```powershell
premake5 --release-optimization=lto vs2022
msbuild Interpreter.sln /m /t:mType:Rebuild /p:Configuration=Release /p:Platform=x64

bin\mType\Release\x64\mType.exe --tests
bin\mType\Release\x64\mType.exe --tests --no-jit
bin\mType\Release\x64\mType.exe --benchmark --benchmark-iterations=7 --benchmark-output=json --jit-stats > perf-lto-jit.jsonl
bin\mType\Release\x64\mType.exe --benchmark --benchmark-iterations=7 --benchmark-output=json --no-jit > perf-lto-nojit.jsonl
```

The LTO result is an independent A/B experiment against the default build. Do
not combine it with source changes or infer a win from binary size or a single
timing sample.

## MSVC PGO experiment

PGO has separate instrument, train, and optimize phases. The instrumented
binary is a data-collection tool, not a performance candidate. `/GENPROFILE`
uses `EXACT` collection so concurrent execution is counted safely, accepting
additional training overhead. The profile database and generated `mType!N.pgc`
files live beside `mType.exe`.

### 1. Remove or archive stale profile data

A `.pgc` file is valid only for the exact instrumented link that created its
`.pgd`. After archiving any profile you need, remove only mType's old files:

```powershell
Remove-Item -Path 'bin\mType\Release\x64\mType!*.pgc' -ErrorAction SilentlyContinue
Remove-Item -LiteralPath 'bin\mType\Release\x64\mType.pgd' -ErrorAction SilentlyContinue
```

### 2. Generate and build the instrumented executable

```powershell
premake5 --release-optimization=pgo-instrument vs2022
msbuild Interpreter.sln /m /t:mType:Rebuild /p:Configuration=Release /p:Platform=x64
```

### 3. Train representative behavior

The commands below provide a reproducible starting profile covering tests,
interpreter execution, JIT compilation, and the canonical benchmark suite.
For a production release, add representative game scripts and weight scenarios
according to real usage; an untrained path may be moved into cold code.

```powershell
$mtype = 'bin\mType\Release\x64\mType.exe'
& $mtype --tests
& $mtype --tests --no-jit
& $mtype --benchmark --benchmark-iterations=7 --jit-stats
& $mtype --benchmark --benchmark-iterations=7 --no-jit

Get-ChildItem -LiteralPath 'bin\mType\Release\x64' -Filter 'mType!*.pgc'
```

Every training process must exit normally so MSVC flushes its counters. If no
`.pgc` file is present, stop; do not create a `pgo-optimize` build with an empty
profile.

### 4. Merge and inspect the profile

Run `pgomgr` from the executable directory so `/merge mType.pgd` finds all
standard-named `mType!N.pgc` files. It is available in the Visual Studio
developer environment.

```powershell
Push-Location 'bin\mType\Release\x64'
pgomgr /merge mType.pgd
pgomgr /summary mType.pgd
Pop-Location
```

Use explicit `pgomgr /merge:n` weighting only when production frequency data
justifies it, and record the weights with the benchmark results.

### 5. Generate and link the optimized executable

Do **not** run `Rebuild` in this phase: a clean can delete the trained `.pgd`.
The compile setting remains `/GL`; regeneration changes the final link from
`/GENPROFILE` to `/USEPROFILE`, which causes MSBuild to relink `mType.exe`.

```powershell
premake5 --release-optimization=pgo-optimize vs2022
msbuild Interpreter.sln /m /t:mType /p:Configuration=Release /p:Platform=x64

bin\mType\Release\x64\mType.exe --tests
bin\mType\Release\x64\mType.exe --tests --no-jit
bin\mType\Release\x64\mType.exe --benchmark --benchmark-iterations=7 --benchmark-output=json --jit-stats > perf-pgo-jit.jsonl
bin\mType\Release\x64\mType.exe --benchmark --benchmark-iterations=7 --benchmark-output=json --no-jit > perf-pgo-nojit.jsonl
```

Treat linker warnings about missing, stale, or mismatched profile data as a
failed experiment. Retrain after any source, compiler, dependency, compile
option, or instrumented-link change.

## Comparison rules and limitations

- Measure default, LTO, and PGO-optimized binaries separately at the same commit.
- Compare PGO both with default (total effect) and LTO (incremental profile effect).
- Preserve each completed executable under an untracked artifact directory if
  interleaved process-level A/B runs are required; regenerating a mode overwrites
  `bin\mType\Release\x64\mType.exe`.
- Require the benchmark policy in [benchmarks.md](benchmarks.md): at least seven
  measured samples per candidate, a median improvement of at least 3%, and
  Welch's two-sided `p < 0.05`; interleave process-level A/B runs when practical.
- Record compiler/toolset versions, commit SHA, training commands, scenario
  weights, binary size, link time, and both JIT and `--no-jit` results.
- PGO currently instruments only the `mType` executable's final link. Launchers,
  the package manager, and language-server executables need separate final-link
  profiles if they become optimization targets.
- `/GL` objects are tied to the compiler toolset that produced them and should
  not be distributed as a stable binary interface.

The configuration follows the current [Premake LTO API](https://premake.github.io/docs/linktimeoptimization)
and Microsoft's documented [native PGO workflow](https://learn.microsoft.com/en-us/cpp/build/profile-guided-optimizations?view=msvc-170),
including the preferred [`/GENPROFILE`](https://learn.microsoft.com/en-us/cpp/build/reference/genprofile-fastgenprofile-generate-profiling-instrumented-build?view=msvc-170)
and `/USEPROFILE` options rather than deprecated `/LTCG:PGINSTRUMENT` and
`/LTCG:PGOPTIMIZE` forms.
