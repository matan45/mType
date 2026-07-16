# mType Interpreter Benchmarks

The benchmark harness measures wall-clock execution time, `ExecutionStats`,
pool counters, and optional JIT/OSR diagnostics for the canonical VM scripts in
`mType/tests/testFiles/benchmarks/`.

## Quick Start

From the repo root:

```powershell
bin\mType\Release\x64\mType.exe --benchmark
```

This runs the 64-script canonical suite with 1 warmup + 3 measured iterations,
JIT on, and prints a human-readable table.

## Flags

| Flag | Default | Meaning |
|---|---|---|
| `--benchmark` | - | Run the full canonical VM benchmark suite. |
| `--benchmark=<path.mt>` | - | Run a single benchmark script. |
| `--benchmark-lexer=<path>` | - | Run the lexer-only token-drain microbenchmark. |
| `--benchmark-iterations=<N>` | `3` | Measured iterations per script. Warmup is fixed at 1. |
| `--benchmark-output=<fmt>` | `text` | `text` or `json`. JSON suppresses script stdout so the stream is parseable. |
| `--no-jit` | JIT on | Disable JIT compilation. Use to capture a pure-interpreter baseline. |
| `--jit-stats` | off | Include JIT/OSR statistics in text output and per-script JSON records. |

## What The Harness Measures

For each iteration it constructs a fresh `ScriptInterpreter`, sets JIT per the
options, then times the full pipeline:

```text
parse -> bytecode compile -> optimize -> execute
<------------- wall-clock measured ------------>
                              <--- exec --->
```

- `wall-clock`: end-to-end `runScript()` duration.
- `exec (VM)`: `ExecutionStats::executionTime`, the inner VM loop only.
- `instructions` / `calls`: first measured iteration counters.
- `string-pool` / `array-pool`: process-singleton deltas across the iteration.
- `bridge-arena` / `object-pool`: bridge-slot and object-instance allocation,
  reuse, return, and discard deltas.
- `gc`: collections, detected cycles, collected objects, tracked allocations,
  suspects, and total collection time attributable to the iteration.
- `jit/code-cache`: compile latency, generated and live native-code bytes, code-cache
  budget rejections, and total/peak reserved JIT frame bytes.
- `jit diagnostics`: with `--jit-stats`, reports compiles, bailouts, cache size, OSR counts,
  hot functions, failed loops, and opcode-level bailout counts.

Across measured iterations the harness reports min / median / mean / stddev of
wall-clock. Use the median for A/B decisions; require at least seven measured
samples per candidate, a 3% delta, and Welch's two-sided `p < 0.05` before
treating a change as a performance result. For build-to-build comparisons,
interleave separate process runs when practical to reduce drift. The minimum
remains useful for spotting one-sided system noise but is not the acceptance
statistic.

## Coverage Notes

The canonical suite is fixed in `BenchmarkRunner_Internal.hpp`; adding a `.mt` file does
not automatically add it to `--benchmark`. Add new deterministic scripts there
and register them in `IntegrationTestSuite.cpp` with a sibling `.expected`.

The suite includes both ordinary allocation workloads and `gc_cycle_churn.mt`,
which creates short-lived cyclic object graphs to exercise GC safepoints and
cycle detection during normal benchmark execution.

`lexer_stress.mt` is intentionally excluded from the VM suite and correctness
registration. Use `--benchmark-lexer=<path>` for lexer-only timing.

## Updating The Baseline

After a meaningful performance change or on a new machine:

```powershell
bin\mType\Release\x64\mType.exe --benchmark --jit-stats > bench.jit.txt
bin\mType\Release\x64\mType.exe --benchmark --no-jit > bench.nojit.txt
```

Copy the numbers into `docs/bench-baseline.md` under a new dated section.
Record machine, commit SHA, build mode, and the exact invocation.

The benchmark scripts' `print(...)` lines are deterministic for a given commit.
If they change, semantics changed, not just performance.
