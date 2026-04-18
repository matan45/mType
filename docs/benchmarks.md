# mType Interpreter Benchmarks

The benchmark harness measures wall-clock execution time and `ExecutionStats`
counters for five canonical scripts. It is the foundation for every perf
story under epic MYT-118 — each optimization is expected to produce a
measurable before/after number against this suite.

## Quick start

From the repo root:

```
mType.exe --benchmark
```

This runs all five scripts in `mType/tests/testFiles/benchmarks/` with
1 warmup + 3 measured iterations each, JIT on, and prints a table.

## Flags

| Flag | Default | Meaning |
|---|---|---|
| `--benchmark` | — | Run the full canonical suite (5 scripts). |
| `--benchmark=<path.mt>` | — | Run a single benchmark script. |
| `--benchmark-iterations=<N>` | 3 | Measured iterations per script. Warmup is fixed at 1. |
| `--benchmark-output=<fmt>` | `text` | `text` (human-readable table) or `json` (one record per script). |
| `--no-jit` | JIT on | Disable JIT compilation. Use to capture a pure-interpreter baseline. |
| `--jit-stats` | off | Print JIT statistics after the benchmark table. |

## What the harness measures

For each iteration it constructs a fresh `ScriptInterpreter`, sets JIT per
the options, then times the full pipeline:

```
parse  ->  bytecode compile  ->  optimize  ->  execute
<-------------- wall-clock measured -------------->
                                 <---- exec ---->
```

- `wall-clock` — end-to-end `runScript()` duration. Matches user-felt latency.
- `exec (VM)` — `ExecutionStats::executionTime`, the inner interpreter loop only.
  The gap between wall-clock and exec is parse/compile cost.
- `instructions` / `calls` — `ExecutionStats::instructionsExecuted` and
  `functionCalls` from the first measured iteration (deterministic per script).
- `mem-allocated` — `ExecutionStats::memoryAllocated` is currently an unused
  stub in the VM and will always read 0 until a future ticket wires it up.
- `string-pool` / `array-pool` — process-singleton deltas observed across the
  iteration.

Across measured iterations the harness reports min / median / mean / stddev
of wall-clock. **Min is the canonical comparison number** — noise is
one-sided upward, so the minimum wall-clock is the most stable estimate.

## Updating the committed baseline

After a meaningful change (or on a new machine), regenerate the baseline:

```
mType.exe --benchmark --benchmark-output=text        > bench.jit.txt
mType.exe --benchmark --no-jit --benchmark-output=text > bench.nojit.txt
```

Copy the numbers into `docs/bench-baseline.md` under a new dated section.
Record machine, commit SHA, and build mode at the top.

The `string_ops`, `method_dispatch`, etc. sanity-output lines
(`print("... acc=...")` etc.) are deterministic across runs for a given
commit — if they change, something about program semantics changed, not
just performance.

## Calibrating iteration counts

Each `.mt` script has a `// TARGET: ~2s` comment near the top.
If your first local run lands outside the 1-5s band for any script,
open the script, scale the loop bound up or down, and commit the new
value alongside an updated `bench-baseline.md`.

## Supersedes

The previous root-level `benchmark.ps1` is superseded by `--benchmark`
and has been removed.
