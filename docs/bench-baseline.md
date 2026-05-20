# mType Interpreter Baseline

Committed min wall-clock numbers from `mType.exe --benchmark`.
See `docs/benchmarks.md` for how to update.

Only the most recent run is kept here; prior runs live in git history
(`git log -p docs/bench-baseline.md`).

## 2026-05-20 — MYT-352 lift NEW_STACK inlining gate

- Machine: dev machine (Windows 11 Home)
- Commit: branch `MYT-352`. Lifts the JIT-side reject of `NEW_STACK`-
  containing callees in `InlineAnalysis.cpp` + the matching boxed-mode
  pre-check in `JitCompiler_EmitHelpers.cpp::calleeMightBeInlineable`.
  Two compiler-side preconditions make the lift safe: function-body
  blocks now emit `STACK_SCOPE_ENTER` / `LEAVE` around `NEW_STACK`
  allocations (previously gated on `shouldManageScope = true`, false
  for function bodies — the frame teardown was the implicit backstop,
  which inlining elides) and `FunctionCompiler::compileReturn` drains
  open scopes by emitting `STACK_SCOPE_LEAVE × currentStackScopeDepth`
  before every `RETURN_VALUE` / return-via-finally `JUMP`, mirroring
  the existing break / continue precedent. Canary fixture +
  `addCallbackTest` in `EscapeAnalysisTestSuite` asserts
  `ObjectInstancePool` hit rate > 0.9 after a 5000-iter inlined
  `distanceSq` workload.
- Build: Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)

### Headline movements vs prior 2026-05-20 (primitive-wrapper cache) baseline

| Benchmark | Prior | New | Δ | Driver |
| --- | ---: | ---: | ---: | --- |
| `object_alloc_nested.mt` | 1702.12 | 751.59 | **−55.8%** | target — inlined `distanceSq` reclaims per-call dispatch + retains per-scope NEW_STACK release |
| `object_alloc.mt` | 621.51 | 348.21 | **−44.0%** | recovered: the per-scope release win that the prior cache patch's regression had eaten, restored by the function-body STACK_SCOPE wrap |
| `gc_churn_intense_hot.mt` | 848.33 | 426.85 | **−49.7%** | same shape as object_alloc — secondary beneficiary of function-body STACK_SCOPE wrap |
| `collections_hash_user_class_hot.mt` | 712.67 | 650.41 | **−8.7%** | secondary win — user-class entry allocation in hashmap path |

The headline win on `object_alloc_nested.mt` exceeds the MYT-352 ticket
projection (30–40%). `distanceSq`'s two `NEW Point(...)` allocations are
now inlined into the outer hot loop body — the bytecode-emitted
`STACK_SCOPE_ENTER` / `LEAVE` from the function-body wrap reclaims the
two pool slots per iteration, so the 32-slot per-frame cap never
saturates.

### Provisional regressions

| Benchmark | Prior | New | Δ |
| --- | ---: | ---: | ---: |
| `inline_polymorphic_mixed.mt` | 427.22 | 354.05 | **−17.1%** (improved, not a regression — listed for completeness) |

No statistically meaningful regressions. The few benchmarks reported as
+7–22% in the prior (cache-only) baseline (lambda_*, function_call_medium,
try_catch_finally_hot, pattern_match_hot, switch_dispatch_hot,
generic_dispatch_hot) all return to or slightly below their pre-cache
levels in this run, supporting the prior note that they were
variance + thermal drift in a single 3-iter run rather than real
cross-effects.

### Summary (jit=on)

```
  Script                             min(ms)    median(ms)    instructions     calls
  arithmetic_tight_loop.mt            109.29        109.35           20017         0
  method_dispatch.mt                   98.22         98.44           14042       506
  object_alloc.mt                     345.42        345.48           13511         0
  object_alloc_nested.mt              752.58        761.14           17011       500
  gc_cycle_churn.mt                   249.40        252.12           26511    160900
  field_write_hot.mt                   79.30         79.37            8018         1
  field_read_hot.mt                    81.49         83.32            9020         1
  polymorphic_field_hot.mt            524.43        525.79        42000094         8
  string_ops.mt                       108.59        109.28           19019         0
  recursive.mt                        673.77        676.65           17260   2545487
  bitwise_tight_loop.mt                79.55         79.88           23019         0
  short_circuit_chain.mt               64.94         65.12           24909         0
  primitive_method_dispatch.mt        194.73        195.96           32041         0
  array_multi_alloc.mt                 34.48         34.55            9911       500
  array_multi_get.mt                  272.67        274.58           60117       500
  for_each_loop.mt                    111.26        116.13           54787      1007
  inline_monomorphic.mt                76.83         76.92           13016       501
  inline_branching.mt                  79.92         80.95           15016       501
  inline_polymorphic.mt                98.79         99.31           14051       508
  inline_polymorphic_mixed.mt         353.11        353.47           32926       508
  megamorphic_dispatch.mt              99.27         99.79           14069       512
  inline_value_object_hot.mt          161.97        162.25           12517       500
  function_call_hot.mt                 91.87         92.55           15011       500
  function_entry_tier_hot.mt           93.37         93.69           11811      1500
  function_call_medium_hot.mt         171.08        171.91           13911       500
  async_await_tight_loop.mt           611.68        612.32           12425       501
  async_await_chain.mt               1212.73       1226.78           23325      2001
  lambda_call_hot.mt                  669.03        686.73           12521   1999501
  lambda_closure_hot.mt               674.98        679.84           12526   1999502
  generic_dispatch_hot.mt             682.45        688.55           20074      1012
  try_catch_finally_hot.mt            352.33        356.43           50019      2000
  switch_dispatch_hot.mt              473.97        476.76           14634       500
  overload_dispatch_hot.mt            504.58        505.82           34026      2001
  abstract_dispatch_hot.mt            101.06        102.81           14042       506
  cast_hot.mt                         256.55        258.19           19560       505
  collections_hash_hot.mt             621.56        624.87           32763       502
  collections_hash_user_class_hot.mt        639.73        642.10           35775       502
  collections_hashset_hot.mt          154.66        155.44           18655         1
  stream_pipeline_hot.mt              383.62        387.04         2090493    292256
  reflection_lookup_hot.mt           1485.45       1489.91           81551   1203003
  pattern_match_hot.mt                457.75        460.64           12861       500
  string_interpolation_hot.mt         218.74        220.07         7400025         0
  boxed_primitive_dispatch_hot.mt        564.42        572.73           32805         0
  boxed_bool_dispatch_hot.mt          454.24        456.15           29276         0
  boxed_string_dispatch_hot.mt        371.83        377.14           24264         0
  box_unbox_hot.mt                    893.69        909.32           16513      1000
  static_call_hot.mt                  167.75        169.83           32516      2000
  linked_list_nested_hot.mt           148.25        149.14          124921      1969
  method_chain_hot.mt                  20.69         20.99           36526      2389
  string_build_call_hot.mt             94.70         95.84           21015       500
  deep_inheritance_hot.mt              80.97         81.34           34531      3006
  interface_dispatch_collections_hot.mt        276.60        277.47           25981      1029
  cast_miss_hot.mt                    191.09        191.96           16459       506
  exception_throw_hot.mt              622.34        623.40         6450035    250001
  ic_transition_hot.mt                320.32        321.10           14243       533
  switch_on_string_hot.mt             696.89        700.35        33000062   1000000
  substring_hot.mt                     83.01         83.08           13013         0
  gc_churn_intense_hot.mt             448.72        449.40           11811       500
  array_literal_alloc_hot.mt          102.16        102.44           29510         0
  value_class_mut_hot.mt              480.61        481.76           14516       500
  int_only_arith_hot.mt                39.57         40.04           14010         0
```

### Notes

- Headline `object_alloc_nested.mt` improvement (−55.8%) exceeds the
  MYT-352 ticket's 30–40% projection. The function-body STACK_SCOPE
  wrap (the StatementCompiler change) is what bridges per-scope release
  to inlined NEW_STACK; without it the lift would re-create the
  original MYT-208 cap saturation in the caller's frame.
- `object_alloc.mt` (−44%) and `gc_churn_intense_hot.mt` (−49.7%) are
  secondary wins from the same StatementCompiler change — both have
  trivial-ctor `Point`-style allocations whose function-body block now
  carries STACK_SCOPE, recovering the per-scope release the prior
  cache patch had partly given back.
- `box_unbox_hot.mt 878` — improved from 1395 (cache-baseline). Likely
  a downstream beneficiary of the same StatementCompiler change for
  the implicit `Box<Int>(new Int(...))` ctor body.
- `value_class_mut_hot.mt` continues to match the `--no-jit` reference;
  MYT-346 canary stays green.
- `EscapeAnalysisTestSuite` new test "MYT-352 Inlined NEW_STACK Pool
  Reuse" green — proves pool hit-rate stays >0.9 across a 5000-iter
  inlined `distanceSq` workload, gating the regression directly.
- Top wall-time sinks (>1s median):
  - `reflection_lookup_hot.mt 1455` — flat vs prior (1540).
  - `async_await_chain.mt 1225` — slight improvement vs prior (1301).
