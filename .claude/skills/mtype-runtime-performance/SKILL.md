---
name: mtype-runtime-performance
description: Measure and improve mType runtime performance across the bytecode VM interpreter and AsmJit JIT. Use when work involves runtime speed, benchmark regressions, interpreter hot paths, VM loop/opcode performance, JIT optimization, OSR, inline caches, deoptimization, --jit-stats, --profile, --profile=full, --benchmark, or comparing JIT output against --no-jit.
---

# mType Runtime Performance

Improve runtime speed by measuring first, preserving correctness, and keeping JIT behavior equivalent to the interpreter reference.

## Core Rule

Do not optimize from intuition alone. Establish a baseline with a repeatable command, capture the signal, make one focused change, then re-run the same command. Runtime work is done only when correctness and measurement both hold.

## 1. Classify the Target

Identify the affected execution path before changing code:

- **Interpreter path**: VM loop, opcode dispatch, runtime executors, profiler hooks, stack/value handling.
- **JIT path**: AsmJit codegen, function-entry tiering, OSR, inline caches, guards, bailout/deopt behavior.
- **Shared path**: runtime values, class/method lookup, arrays, strings, async, call dispatch, native bridges.

Start in the likely files:

- `mType/vm/runtime/VirtualMachineLoop.cpp`
- `mType/vm/runtime/VirtualMachineJit.cpp`
- `mType/vm/jit/**`
- `mType/tests/testFiles/benchmarks/**`
- `mType/tests/suites/IntegrationTestSuite.cpp`

## 2. Build the Measurement Loop

Prefer the smallest command that exercises the real hot path:

- Use `mType --benchmark` for broad benchmark coverage.
- Use `mType --benchmark=<path-to-benchmark.mt>` for targeted benchmark work.
- Use `mType --profile <script.mt>` for function timing.
- Use `mType --profile=full <script.mt>` for timing, call graph, and opcode counts.
- Use `mType --jit-stats <script.mt>` when judging JIT compilation, bailout, inline-cache, OSR, or inlining behavior.
- Use `mType --no-jit <script.mt>` as the interpreter reference.

For JIT performance work, always compare at least:

```text
mType --no-jit <script.mt>
mType --jit-stats <script.mt>
```

Capture the relevant output before changing code: elapsed time, profiler totals, opcode counts, compile counts, bailout reasons, inline-cache counters, or output mismatches.

## 3. Preserve Correctness

Treat `--no-jit` as the semantic reference unless the task proves an interpreter bug. JIT-on output must match the `--no-jit` reference for the same script.

For benchmark-backed JIT correctness, use the existing convention:

- Benchmark fixtures live under `mType/tests/testFiles/benchmarks/`.
- `.expected` files should reflect the `--no-jit` reference output.
- Integration coverage is registered in `mType/tests/suites/IntegrationTestSuite.cpp`.

If a change affects execution semantics, add or update an output verification test before trusting a speedup.

## 4. Optimize Narrowly

Use the measurement to choose one change at a time:

- For interpreter changes, prefer reducing hot-loop work, avoiding repeated lookups, preserving profiler behavior, and keeping opcode semantics local.
- For JIT changes, prefer better eligibility checks, lower fallback overhead, clearer bailout reasons, and preserving deopt paths.
- For inline-cache or type-feedback changes, verify mono/poly/mega behavior with stats and a targeted fixture.
- For call dispatch changes, verify direct calls, method calls, recursive calls, async/deopt cases, and JIT-to-JIT paths when relevant.

Avoid broad rewrites unless the measurement proves the cost is structural and a small change cannot address it.

## 5. Validate the Result

Before declaring success:

- Re-run the original baseline command and report before/after numbers.
- Re-run the equivalent `--no-jit` correctness check for JIT work.
- Run the narrowest relevant test suite or integration fixture.
- Run both JIT-on and `--no-jit` test paths when the change touches shared runtime semantics.
- Confirm any added debug instrumentation is removed.

When reporting results, include the command, the old signal, the new signal, and any remaining risk.
