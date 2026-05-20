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
  arithmetic_tight_loop.mt            106.79        107.01           20017         0
  method_dispatch.mt                   98.72        100.64           14042       506
  object_alloc.mt                     348.21        350.36           13511         0
  object_alloc_nested.mt              751.59        752.20           17011       500
  gc_cycle_churn.mt                   245.13        247.09           26511    160900
  field_write_hot.mt                   77.82         77.96            8018         1
  field_read_hot.mt                    80.88         81.28            9020         1
  polymorphic_field_hot.mt            518.44        519.11        42000094         8
  string_ops.mt                       108.59        108.65           19019         0
  recursive.mt                        677.91        678.09           17260   2545487
  bitwise_tight_loop.mt                77.09         77.83           23019         0
  short_circuit_chain.mt               63.12         63.88           24909         0
  primitive_method_dispatch.mt        193.90        194.48           32041         0
  array_multi_alloc.mt                 34.94         35.47            9911       500
  array_multi_get.mt                  272.50        272.89           60117       500
  for_each_loop.mt                    110.56        112.65           54787      1007
  inline_monomorphic.mt                75.67         76.12           13016       501
  inline_branching.mt                  77.45         77.75           15016       501
  inline_polymorphic.mt                96.08         97.52           14051       508
  inline_polymorphic_mixed.mt         354.05        354.57           32926       508
  megamorphic_dispatch.mt              96.76         96.99           14069       512
  inline_value_object_hot.mt          162.90        163.24           12517       500
  function_call_hot.mt                 92.04         92.25           15011       500
  function_entry_tier_hot.mt           92.50         92.62           11811      1500
  function_call_medium_hot.mt         167.57        168.32           13911       500
  async_await_tight_loop.mt           608.13        609.90           12425       501
  async_await_chain.mt               1217.95       1225.49           23325      2001
  lambda_call_hot.mt                  653.55        656.83           12521   1999501
  lambda_closure_hot.mt               664.63        667.44           12526   1999502
  generic_dispatch_hot.mt             650.51        653.56           20074      1012
  try_catch_finally_hot.mt            343.53        344.05           50019      2000
  switch_dispatch_hot.mt              466.50        467.45           14634       500
  overload_dispatch_hot.mt            492.25        494.07           34026      2001
  abstract_dispatch_hot.mt             95.96         97.38           14042       506
  cast_hot.mt                         246.75        248.95           19560       505
  collections_hash_hot.mt             638.12        639.36           32763       502
  collections_hash_user_class_hot.mt        650.41        654.14           35775       502
  collections_hashset_hot.mt          150.58        151.57           18655         1
  stream_pipeline_hot.mt              375.59        379.57         2090493    292256
  reflection_lookup_hot.mt           1443.76       1455.67           81551   1203003
  pattern_match_hot.mt                451.28        452.77           12861       500
  string_interpolation_hot.mt         215.77        216.64         7400025         0
  boxed_primitive_dispatch_hot.mt        555.40        559.32           32805         0
  boxed_bool_dispatch_hot.mt          440.37        443.67           29276         0
  boxed_string_dispatch_hot.mt        365.43        367.09           24264         0
  box_unbox_hot.mt                    878.55        879.54           16513      1000
  static_call_hot.mt                  166.01        166.56           32516      2000
  linked_list_nested_hot.mt           145.67        147.95          124921      1969
  method_chain_hot.mt                  20.15         20.90           36526      2389
  string_build_call_hot.mt             94.31         95.17           21015       500
  deep_inheritance_hot.mt              79.57         79.84           34531      3006
  interface_dispatch_collections_hot.mt        276.87        279.24           25981      1029
  cast_miss_hot.mt                    187.63        188.01           16459       506
  exception_throw_hot.mt              626.09        626.24         6450035    250001
  ic_transition_hot.mt                316.88        318.15           14243       533
  switch_on_string_hot.mt             685.59        687.41        33000062   1000000
  substring_hot.mt                     81.71         82.79           13013         0
  gc_churn_intense_hot.mt             426.85        427.03           11811       500
  array_literal_alloc_hot.mt          100.64        102.80           29510         0
  value_class_mut_hot.mt              487.76        488.12           14516       500
  int_only_arith_hot.mt                37.67         37.84           14010         0
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
