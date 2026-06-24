# Performance Optimization — Reference

Enumerable detail for [SKILL.md](SKILL.md): pass inventories, value-layer catalog, the C++ A/B recipe, and the CLI-flag table.

## AST optimizer passes

Pipeline: `mType/optimizer/Optimizer` → `OptimizationPassManager` (runs to a **fixed point**, bounded by `OptimizationConfig::maxPassIterations` + `timeoutPerPass`). Wired to `forRelease()` at `mType/services/OptimizationService.cpp:10`.

Registration order (`OptimizationPassManager.cpp`) — order matters and is commented in-source:

| Order | Pass | File | Config flag (`OptimizationConfig`) |
|---|---|---|---|
| 1 | `LombokSynthesisPass` | `optimizer/passes/LombokSynthesisPass.hpp` | `setLombokSynthesis(bool)` |
| 2 | `StructuralEqualitySynthesisPass` (MYT-274) | `optimizer/passes/StructuralEqualitySynthesisPass.hpp` | `setStructuralEqualitySynthesis(bool)` |
| 3 | `ConstantFoldingPass` | `optimizer/passes/ConstantFoldingPass.hpp` | `setConstantFolding(bool)` |
| 4 | `DeadCodeEliminationPass` | `optimizer/passes/DeadCodeEliminationPass.hpp` | `setDeadCodeElimination(bool)` |
| 5 | `EscapeAnalysisPass` (MYT-134) | `optimizer/passes/EscapeAnalysisPass.hpp` | `setEscapeAnalysis(bool)` |

Ordering rationale: synthesis passes inject members first; folding exposes dead branches before DCE removes them; escape analysis runs last so earlier passes have already pruned fake-escape branches. Factory methods: `OptimizationConfig::forRelease()` (all on) and `OptimizationConfig::noOptimization()` (all off). DCE respects `@Script`/`@Throw` annotations — do not strip annotated declarations.

## Bytecode optimizer passes

Pipeline: `mType/vm/optimization/BytecodeOptimizer` → `BytecodeOptimizationPassManager` (**single pass, in place** on `BytecodeProgram` — no fixed point). Wired to `forRelease()` at `mType/services/BytecodeOptimizationService.cpp:8`.

Registration order (`BytecodeOptimizationPassManager.cpp`):

| Order | Pass | File | Config flag (`BytecodeOptimizationConfig`) |
|---|---|---|---|
| 1 | `LoopOptimizationPass` | `vm/optimization/passes/LoopOptimizationPass.hpp` | `setLoopOptimization(bool)` |
| 2 | `PeepholeOptimizationPass` | `vm/optimization/passes/PeepholeOptimizationPass.hpp` | `setPeephole(bool)` |
| 3 | `TrivialSetterInliningPass` | `vm/optimization/passes/TrivialSetterInliningPass.hpp` | `setTrivialSetterInlining(bool)` |
| 4 | `TrivialGetterInliningPass` | `vm/optimization/passes/TrivialGetterInliningPass.hpp` | `setTrivialGetterInlining(bool)` |
| 5 | `LocalArrayFusionPass` | `vm/optimization/passes/LocalArrayFusionPass.hpp` | `setLocalArrayFusion(bool)` |

Peephole patterns live under `vm/optimization/patterns/` (ConstantFolding, AlgebraicSimplification, StrengthReduction, JumpThreading, DeadCode, StackOptimization, TypeSpecialization, NullSequence, Superinstruction, AbstractClass) and run through `PeepholeOptimizer` + the `vm/optimization/analysis/` helpers (ControlFlow/DataFlow/JumpTarget). Inlining decisions go through `vm/optimization/InlineAnalysis.hpp`. `BytecodeOptimizationConfig::isPassEnabled(name)` defaults unknown names to `true`, so a future pass is opt-out.

The pass manager already computes per-pass `elapsed` timing but currently discards it (`(void)elapsed`) — that is the cheapest instrumentation seam if you need "which pass costs what."

## Value / memory layer catalog

All headers under `mType/value/`.

| Structure | Header | Amortizes | Failure mode / watch |
|---|---|---|---|
| `StringPool` | `StringPool.hpp` | interning; O(1) interned-vs-interned compare | intern misses on dynamic strings; ref-count churn |
| `InternedString` | `InternedString.hpp` | interned-string handle | comparing interned vs non-interned |
| `StringCache` | `StringCache.hpp` | small/common string reuse | — |
| `ArrayPool` | `ArrayPool.hpp` | array alloc by dimension signature | sizes outside pooled set fall through to malloc |
| `FlatMultiArray` | `FlatMultiArray.hpp` | contiguous N-D storage, stride indexing | sub-arrays are **views**; row replacement raises MT-E5005 |
| `SparseMultiArray` | `SparseMultiArray.hpp` | sparse N-D storage | — |
| `IntegerCache` / `FloatCache` / `BoolCache` | `IntegerCache.hpp` / `FloatCache.hpp` / `BoolCache.hpp` | boxed-primitive reuse | boxing churn; verify with `box_unbox_hot.mt` |
| `ObjectInstancePool` | `ObjectInstancePool.hpp` | short-lived object reuse | only helps when escape analysis proves non-escape |
| `SmallArgsBuffer` | `SmallArgsBuffer.hpp` | small call-arg arrays without heap | large arg counts spill |

Rule from SKILL.md: a pool/cache change must show **both** a `--gc-stats` delta and a wall-time delta.

## C++ A/B recipe (no CLI flag)

There is no `--no-optimize`. To measure one pass in isolation:

1. **Capture baseline** with the current (`forRelease()`) build via `perf-compare.ps1 -Mode baseline`.
2. **Disable the one pass.** Edit the service that wires the config:
   - AST: `mType/services/OptimizationService.cpp:10` — replace `optimizer::OptimizationConfig::forRelease()` with `optimizer::OptimizationConfig::forRelease().setEscapeAnalysis(false)` (or the relevant `set*` flag).
   - Bytecode: `mType/services/BytecodeOptimizationService.cpp:8` — `vm::optimization::BytecodeOptimizationConfig::forRelease().setPeephole(false)` (or relevant flag).
3. **Ask the user to rebuild Release** (don't build yourself) and re-run `perf-compare.ps1 -Mode compare`.
4. The delta is that pass's contribution. **Revert the edit** before landing — the config change is a measurement tool, not a shipped change.

Tests already do this in isolation: see `mType/tests/suites/BytecodeOptimizationTestSuite.cpp`, which starts from `noOptimization()` and flips one flag on per case.

## CLI flag reference (triage roles)

Authoritative source: `mType/run/commands/MainCommandsExecute.cpp`.

| Flag | Triage role |
|---|---|
| `--profile` / `--profile=light` | function timing (light) |
| `--profile=full` | parse/compile/optimize vs execute split + call graph + opcode histogram — start here |
| `--profile-output=json` | machine-readable profile for cross-build diffing |
| `--gc-stats` | allocation counts → routes to the value/memory layer |
| `--jit-stats` | is the JIT firing? routing signal only; deep use → runtime-perf |
| `--no-jit` | interpreter-only baseline + MYT-259 correctness oracle |
| `--no-stack-scope-release` | treat `STACK_SCOPE_ENTER/LEAVE` as no-ops to A/B stack-scope release |
| `--benchmark` / `--benchmark=<file>` | run all / one benchmark |
| `--benchmark-iterations=N` | measured iteration count (noise averaging) |
| `--benchmark-output=text\|json` | benchmark report format |
| `--compile` | emit `.mtc` bytecode (inspect what the optimizers produced) |

Optimizer toggles (`forRelease()` / `noOptimization()` / per-pass `set*`) are **C++/test-only** — not exposed as CLI flags.
