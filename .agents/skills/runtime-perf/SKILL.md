---
name: runtime-perf
description: Measurement-first discipline for improving mType interpreter and JIT performance, and triaging perf regressions. Use when the user asks to speed up the interpreter/JIT, optimize a hot path (executors, ICs, OSR, escape analysis, GC), investigate a JIT canary divergence, or chase a MYT-* perf ticket. Pairs with [benchmark-coverage](../benchmark-coverage/SKILL.md) when the work needs new benchmarks first.
---

# Runtime Perf

Perf work without measurement is fiction. Every change is justified by a before/after number on a benchmark that exercises the path you touched.

## Hard rules

- **Always Release build** (`bin/mType/Release/x64/mType.exe`). Debug numbers mean nothing.
- **Verify correctness against `--no-jit`** before celebrating any speedup. JIT-on output must match the `.expected` captured from `--no-jit` (MYT-259 regression contract).
- **Don't run premake5/msbuild** — the user builds and reports back ([feedback_user_runs_build](../../../memory/feedback_user_runs_build.md)). Wait for the green binary, then measure.
- **One variable per patch.** Mixing a fast-path change with refactoring poisons the measurement.

## Workflow

### 1. Pick the benchmark before touching code

Find the `.mt` in `mType/tests/testFiles/benchmarks/` that exercises your target path. The file header comments name the MYT ticket and the opcode/helper under test (e.g. `field_read_hot.mt` calls out `GET_FIELD_CACHED`). If no benchmark covers it, **stop and run [benchmark-coverage](../benchmark-coverage/SKILL.md) first** — you cannot measure what you don't run.

### 2. Capture baseline

```powershell
.\.Codex\skills\runtime-perf\scripts\perf-compare.ps1 `
    -Benchmark mType\tests\testFiles\benchmarks\field_read_hot.mt `
    -Mode baseline -Iterations 7
```

Saves a JSON snapshot under `.perf-baselines/`. Use `-Mode compare` after the patch to print delta + verdict.

For correctness-only sanity, prefer the existing `--tests` and `--tests --no-jit` runs over building a one-off.

### 3. Profile before guessing

- `--jit-stats <file>` — IC hit rate, OSR fires, deopt count, code-cache size. First thing to check for any JIT change.
- `--profile=full <file>` — function timing + call graph + opcode histogram. Tells you whether your hot path is actually hot.
- `--profile-output=json` — machine-readable; pipe into the harness for differential profiling.
- `--no-jit` baseline — confirms whether the regression is in the interpreter, the JIT, or both.

### 4. Hypothesize from the JIT topology, not the source

Anchors (`mType/vm/jit/`):

- **`JitCompiler_*.cpp`** — emit-side. Per-opcode codegen split by category (Arith, Calls, Objects, Arrays, ControlFlow, Locals, Returns).
- **`JitHelpers_*.cpp`** — runtime callbacks invoked via `cc.invoke`. Cost ~300 cycles per call ([project_jit_tiny_method_ceiling](../../memory/project_jit_tiny_method_ceiling.md)).
- **`ic/`** — inline caches; MONO/POLY/MEGA transitions. Watch `IC_MAX_POLYMORPHIC_ENTRIES` overflow.
- **`OSRManager*` / `JitCompiler_OSR*`** — on-stack replacement. Past divergences ([project_myt346_value_class_osr_divergence](../../memory/project_myt346_value_class_osr_divergence.md)) are canary-tracked.
- **`profiler/`** — function-entry tiering, loop hotness, type-feedback collection.

Always check the [memory index](../../../memory/MEMORY.md) for landed perf work and known pitfalls before designing a fix — many obvious-looking wins have been tried and have a tradeoff documented (see project_myt199_scaffolding_only, project_myt173_cached_opcode, project_jit_phases_done).

### 5. Measure delta with significance

`perf-compare.ps1 -Mode compare` reports mean, median, stddev, p-value (Welch's t-test approximation), and verdict:

- **IMPROVEMENT** — median ≥ 3% faster, p < 0.05.
- **REGRESSION** — median ≥ 3% slower, p < 0.05.
- **NOISE** — anything else; do not ship a "1% improvement" patch.

Re-run with `-Iterations 15` if the variance band straddles your delta. Iterations are cheap; wrong conclusions are not.

### 6. Sweep adjacent benchmarks

A patch to `JitCompiler_Calls.cpp` should be measured against every `*_dispatch_hot`, `*_call_*`, `inline_*`, and `method_*` benchmark — not just the one you targeted. The script's `-Suite` mode runs an `.mt` glob and aggregates.

### 7. Land checklist

- [ ] `--tests` and `--tests --no-jit` both green.
- [ ] Baseline → patched delta archived under `.perf-baselines/` and quoted in the commit/PR body.
- [ ] No new `[DEBUG-*]` instrumentation left in code.
- [ ] If the win depended on a non-obvious invariant (e.g. clobber rules from [feedback_jit_no_cache_across_cc_invoke](../../memory/feedback_jit_no_cache_across_cc_invoke.md)), capture it as a feedback memory.
- [ ] If a hypothesis was wrong, leave one line in the PR description explaining what didn't work — saves the next person from trying it.

## Regression triage

When a benchmark slows down or a JIT canary diverges from its `.expected`:

1. `git log --oneline mType/vm/jit/ mType/vm/runtime/` between last-known-good and HEAD — narrow the window.
2. `git bisect run` with `perf-compare.ps1 -Mode bisect` (returns non-zero on regression).
3. For correctness divergences: diff the `--jit` stdout against `--no-jit` stdout on the failing benchmark. Even when both succeed at the script level, the print outputs can drift (typical OSR / value-class bug).
4. File the finding to MYT (see [reference_jira_myt](../../../memory/reference_jira_myt.md)) before patching — perf regressions deserve a ticket even when same-day fixed.

## Anti-patterns

- "Looks faster on my machine" without a baseline JSON. Discard.
- Optimizing a path the profiler shows is <1% of wall time.
- Folding a refactor into the perf patch. Two commits, two measurements.
- Caching a virtreg across `cc.invoke` — clobber/spill makes it stale ([feedback_jit_no_cache_across_cc_invoke](../../memory/feedback_jit_no_cache_across_cc_invoke.md)).
- Returning `shared_ptr` by value from a hot helper out of a `.cpp` TU — 2 atomic refcount ops per dispatch ([project_myt200_sharedptr_returnbyvalue_cost](../../memory/project_myt200_sharedptr_returnbyvalue_cost.md)).
- Skipping `--no-jit` correctness verification because "it's only a perf tweak". Several past JIT bugs survived in this exact rationalization.
