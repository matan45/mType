# mType Interpreter Benchmarks

The benchmark harness measures wall-clock execution time, `ExecutionStats`,
pool counters, and optional JIT/OSR diagnostics for the canonical VM scripts in
`mType/tests/testFiles/benchmarks/`.

## Quick Start

From the repo root:

```powershell
bin\mType\Release\x64\mType.exe --benchmark
```

This runs the 44-script canonical suite with 1 warmup + 3 measured iterations,
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
- `jit`: with `--jit-stats`, reports compiles, bailouts, cache size, OSR counts,
  hot functions, failed loops, and opcode-level bailout counts.

Across measured iterations the harness reports min / median / mean / stddev of
wall-clock. Min is the canonical comparison number because noise is usually
one-sided upward.

## Coverage Notes

The canonical suite is fixed in `BenchmarkRunner.cpp`; adding a `.mt` file does
not automatically add it to `--benchmark`. Add new deterministic scripts there
and register them in `IntegrationTestSuite.cpp` with a sibling `.expected`.

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
