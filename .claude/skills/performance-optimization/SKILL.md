---
name: performance-optimization
description: Whole-engine performance triage for the mType bytecode VM + JIT — find which subsystem is slow, then optimize the compile-time optimizer passes and the value/memory layer. Use when you don't yet know where the cost is, when "make mType faster" is the ask, when an AST/bytecode optimizer pass regresses or needs adding, or when allocation/interning/pooling is the bottleneck. Pairs with [runtime-perf](../runtime-perf/SKILL.md) for hot interpreter/JIT paths and [benchmark-coverage](../benchmark-coverage/SKILL.md) for benchmark gaps.
---

# Performance Optimization

Find *where* the engine spends time before optimizing *how*. This skill is the entry point for "make mType faster" — it triages the whole pipeline, then owns the two altitudes `runtime-perf` skips: the compile-time optimizer passes and the value/memory layer. Hot interpreter/JIT codegen routes to [runtime-perf](../runtime-perf/SKILL.md).

## Hard rules

- **Always Release** (`bin/mType/Release/x64/mType.exe`). Debug numbers mean nothing.
- **Every change is justified by a before/after number.** Reuse the existing harness — `runtime-perf/scripts/perf-compare.ps1`. Do not author a second one.
- **Verify correctness against `--no-jit`** (MYT-259 contract) before celebrating a speedup. For optimizer-pass changes, **also** verify output is unchanged against a `noOptimization()` build — a pass that changes results is a bug, not a win.
- **Don't run premake5/msbuild** — the user builds and reports back ([feedback_user_runs_build](../../memory/feedback_user_runs_build.md)). Wait for the green binary, then measure.
- **One variable per patch.** Folding a refactor into a perf change poisons the measurement.

## Triage: where is the cost?

Reproduce in Release first: `--benchmark=<file> --benchmark-iterations=7`. Then walk the tree — every leaf names the owning section or skill.

```
mType program slow / regressed. Where is the cost?

0. Reproduce in Release. No benchmark hits the path? -> benchmark-coverage FIRST, then return.

1. COMPILE-TIME vs RUNTIME?   --profile=full splits parse/compile/optimize vs execute.
   |- compile/optimize dominates -> AST or bytecode optimizer pass -> THIS SKILL, "Optimizer passes".
   |- execute dominates         -> step 2.

2. Is it the interpreter or the JIT?   --jit-stats, then compare vs --no-jit.
   |- much slower only with --no-jit  -> interpreter executor / dispatch  -> route to runtime-perf.
   |- jit ~= no-jit on a hot loop     -> not tiering / bailing out         -> runtime-perf (OSR/tiering/deopt).
   |- jit slower OR output differs    -> canary divergence                 -> runtime-perf regression triage.

3. ALLOCATION / MEMORY pressure?   --gc-stats
   |- short-lived objects   -> EscapeAnalysisPass + ObjectInstancePool   -> THIS SKILL, "Value/memory layer".
   |- string churn          -> StringPool / InternedString / StringCache -> "Value/memory layer".
   |- array / box churn     -> ArrayPool, FlatMultiArray views, primitive caches -> "Value/memory layer".
   |- stack-scope suspect   -> A/B with --no-stack-scope-release.
```

## Measurement workflow

```powershell
.\.claude\skills\runtime-perf\scripts\perf-compare.ps1 `
    -Benchmark mType\tests\testFiles\benchmarks\<file>.mt `
    -Mode baseline -Iterations 7
# ...apply one change, rebuild (user) ...
.\.claude\skills\runtime-perf\scripts\perf-compare.ps1 `
    -Benchmark mType\tests\testFiles\benchmarks\<file>.mt -Mode compare
```

Verdicts are the same as `runtime-perf`: **IMPROVEMENT** (median ≥3% faster, p<0.05), **REGRESSION** (≥3% slower, p<0.05), **NOISE** (everything else — don't ship a "1% win"). Use `-Suite` to sweep an `.mt` glob of related benchmarks.

Profiling signals:
- `--profile=full` — the parse/compile/optimize vs execute split + call graph + opcode histogram. The first thing to look at; it decides which branch of the triage tree you're on.
- `--profile-output=json` — machine-readable; diff across builds.
- `--gc-stats` — allocation counts; the routing signal for the value/memory layer.
- `--jit-stats` — here it is only a *routing signal* (is the JIT even firing?). Deep IC/OSR/deopt reading belongs to [runtime-perf](../runtime-perf/SKILL.md).

**There is no CLI flag to disable the optimizers.** To A/B a single pass you flip its flag in `OptimizationConfig` / `BytecodeOptimizationConfig`, rebuild, and capture two baselines. See [REFERENCE.md](REFERENCE.md) for the exact recipe.

## Optimizer passes (compile-time — owned here)

Two pipelines run before execution. Pass inventories, flag names, and the A/B recipe are in [REFERENCE.md](REFERENCE.md).

- **AST passes** (`mType/optimizer/passes/`) run to a **fixed point** (`OptimizationPassManager`, bounded by `maxPassIterations`). Order is deliberate: Lombok → StructuralEquality → ConstantFolding → DeadCodeElimination → EscapeAnalysis — Lombok injects members first, folding exposes dead branches before DCE, and escape analysis runs last so earlier passes have already pruned fake-escape branches ([project_lombok_synthesis](../../memory/project_lombok_synthesis.md), [project_myt352_inlining_lift](../../memory/project_myt352_inlining_lift.md)).
- **Bytecode passes** (`mType/vm/optimization/passes/`) run **single-pass, in place** on the `BytecodeProgram` (`BytecodeOptimizationPassManager`): Loop → Peephole → TrivialSetterInlining → TrivialGetterInlining → LocalArrayFusion.

A new or changed pass must: (a) be net-positive **end-to-end**, not just on one program — sweep the suite, watch compile time too; (b) preserve output against **both** `--no-jit` and a `noOptimization()` build; (c) be gated by a config flag so it can be A/B'd and disabled. Note mType has no runtime metaprogramming — member synthesis (Lombok-style) is a C++ optimizer pass, never a `.mt` library ([project_no_runtime_metaprogramming](../../memory/project_no_runtime_metaprogramming.md)).

## Value/memory layer (owned here)

When `--gc-stats` says allocation is the cost, the lever is here. Full catalog (header path, related benchmark, failure mode) in [REFERENCE.md](REFERENCE.md).

| Structure | Amortizes | Bottleneck signal |
|---|---|---|
| `StringPool` / `InternedString` / `StringCache` | string identity + O(1) compare | string churn, intern misses |
| `ArrayPool` | array allocation by dimension signature | array alloc/free in a loop |
| `FlatMultiArray` / `SparseMultiArray` | contiguous N-D storage; sub-arrays are **views** | view materialized in a hot path |
| `IntegerCache` / `FloatCache` / `BoolCache` | boxed-primitive reuse | boxing churn (`box_unbox_hot.mt`) |
| `ObjectInstancePool` | short-lived object reuse (with escape analysis) | high object alloc rate |

**Rule:** a pool/cache change needs **both** a `--gc-stats` delta AND a wall-time delta. Fewer allocations that don't move wall time is not a win. Beware view semantics: `K[][]` sub-arrays are aliases, not copies ([project_flatmultiarray_no_row_replacement](../../memory/project_flatmultiarray_no_row_replacement.md)).

## Runtime / JIT (routes out)

Hot interpreter executors, inline caches (MONO/POLY/MEGA), OSR, tiering, deopt, type feedback, and JIT canary divergences all live in [runtime-perf](../runtime-perf/SKILL.md). Don't duplicate the JIT topology here — triage to that skill once the tree above points at execution.

## Anti-patterns

- Inventing a `--no-optimize` flag. It doesn't exist — A/B via config + rebuild.
- Claiming a win from a `--gc-stats` improvement with no wall-time delta.
- A pass that wins on one program but regresses suite-wide compile time or fixed-point convergence.
- Optimizing something `--profile=full` shows is <1% of cost.
- Materializing a `FlatMultiArray` view in a hot path instead of indexing through it.
- Changing a `Value`/value-struct layout without re-verifying `--no-jit` output — boxing/identity bugs hide here ([feedback_value_bool_pointer_decay](../../memory/feedback_value_bool_pointer_decay.md)).
- Folding a refactor into the perf patch. Two commits, two measurements.

## Boundaries

- **[runtime-perf](../runtime-perf/SKILL.md)** owns JIT codegen/helpers, inline caches, OSR/tiering/deopt, type feedback, hot interpreter executors, and canary-divergence triage.
- **[benchmark-coverage](../benchmark-coverage/SKILL.md)** owns `.mt`+`.expected` authoring, `IntegrationTestSuite.cpp` registration, and coverage-gap audits.
- **This skill** owns the triage that decides which of those to use, the compile-time optimizer passes, and the value/memory layer.

## See Also

- [REFERENCE.md](REFERENCE.md) — pass inventories, value-layer catalog, C++ A/B recipe, CLI-flag table.
- [runtime-perf](../runtime-perf/SKILL.md), [benchmark-coverage](../benchmark-coverage/SKILL.md), [diagnose](../diagnose/SKILL.md) (for git-bisecting a regression).

## Verification (land gate)

- [ ] `--tests` and `--tests --no-jit` both green.
- [ ] Optimizer-pass changes also match a `noOptimization()` build's output.
- [ ] Baseline → patched delta archived via `perf-compare.ps1` with a real verdict (not NOISE), quoted in the PR.
- [ ] Value-layer changes show `--gc-stats` **and** wall-time gains.
- [ ] `--profile=full` confirms the thing you optimized was >1% of cost.
- [ ] No leftover `[DEBUG-*]` instrumentation; any temporary timing hook restored.
