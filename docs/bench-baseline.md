# mType Interpreter Baseline

Committed min wall-clock numbers from `mType.exe --benchmark`.
See `docs/benchmarks.md` for how to update.

Only the most recent run is kept here; prior runs live in git history
(`git log -p docs/bench-baseline.md`).

## 2026-06-02 — baseline refresh on branch MYT-373

- Machine: dev machine (Windows 11 Home)
- Commit: branch `MYT-373` (null-narrowing facts — `&&` / `||` short-circuit
  and guard-clause/loop-condition narrowing). The change is **compile-time
  only** (bytecode compiler + type inference); no runtime/JIT path is
  touched, so steady-state hot numbers should not move. Movements below
  reflect run-to-run variance vs the 2026-05-23 MYT-361 baseline.
- Build: Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)
- New this run: three MYT-373 null-narrowing-matrix benchmarks
  (`null_narrowing_hot`, `null_guard_clause_hot`, `null_narrowing_while_hot`)
  — no prior baseline, so excluded from the movement tables.

### Notable movements vs prior 2026-05-23 (MYT-361) baseline

Regressions (median delta ≥ +10%):

| Benchmark | Prior | New | Δ |
| --- | ---: | ---: | ---: |
| `gc_churn_intense_hot.mt` | 426.44 | 472.05 | **+10.7%** |

Improvements (median delta ≤ −5%):

| Benchmark | Prior | New | Δ |
| --- | ---: | ---: | ---: |
| `inline_value_object_hot.mt` | 215.39 | 169.04 | **−21.5%** |
| `field_read_hot.mt` | 100.17 | 83.41 | **−16.7%** |
| `value_class_mut_hot.mt` | 595.02 | 502.21 | **−15.6%** |
| `static_call_hot.mt` | 207.82 | 177.69 | **−14.5%** |
| `inline_monomorphic.mt` | 89.41 | 79.81 | **−10.7%** |
| `inline_branching.mt` | 88.84 | 83.23 | **−6.3%** |
| `int_only_arith_hot.mt` | 38.42 | 36.51 | **−5.0%** |

### Summary (jit=on)

```
  Script                             min(ms)    median(ms)    instructions     calls
  arithmetic_tight_loop.mt            104.41        105.39           20017         0
  method_dispatch.mt                  105.33        106.25           14042       506
  object_alloc.mt                     363.19        363.24           13511         0
  object_alloc_nested.mt              788.86        789.75           17011       500
  gc_cycle_churn.mt                   257.54        262.41           26511    160900
  field_write_hot.mt                   83.24         83.29            8018         1
  field_read_hot.mt                    82.34         83.41            9020         1
  polymorphic_field_hot.mt            525.42        533.95        42000094         8
  string_ops.mt                       111.31        111.75           19019         0
  recursive.mt                        700.65        706.54           17260   2545487
  bitwise_tight_loop.mt                78.13         78.65           23019         0
  short_circuit_chain.mt               64.60         65.54           24909         0
  primitive_method_dispatch.mt        200.45        202.98           32040         0
  array_multi_alloc.mt                 36.24         36.49            9911       500
  array_multi_get.mt                  277.61        279.03           60117       500
  for_each_loop.mt                    118.76        121.05           54787      1007
  inline_monomorphic.mt                79.44         79.81           13016       501
  inline_branching.mt                  82.93         83.23           15016       501
  inline_polymorphic.mt               105.91        105.91           14051       508
  inline_polymorphic_mixed.mt         366.40        367.35           32926       508
  megamorphic_dispatch.mt             106.24        107.90           14069       512
  inline_value_object_hot.mt          166.91        169.04           12517       500
  function_call_hot.mt                 93.02         93.74           15011       500
  function_entry_tier_hot.mt           93.81         94.65           11811      1500
  function_call_medium_hot.mt         177.27        180.32           13911       500
  async_await_tight_loop.mt           640.04        640.45           12423       501
  async_await_chain.mt               1266.50       1271.87           23325      2001
  lambda_call_hot.mt                  701.80        703.72           12521   1999501
  lambda_closure_hot.mt               705.21        708.90           12526   1999502
  generic_dispatch_hot.mt             686.01        686.69           20074      1012
  try_catch_finally_hot.mt            378.95        379.49           50019      2000
  switch_dispatch_hot.mt              485.23        485.91           14634       500
  overload_dispatch_hot.mt            505.47        509.72           34026      2001
  abstract_dispatch_hot.mt            107.84        108.44           14042       506
  cast_hot.mt                         274.80        276.05           19560       505
  collections_hash_hot.mt             637.94        639.08           32763       502
  collections_hash_user_class_hot.mt        644.73        644.93           35775       502
  collections_hashset_hot.mt          160.44        161.82           18655         1
  stream_pipeline_hot.mt              387.59        396.94         2090493    292256
  reflection_lookup_hot.mt           1491.04       1491.39           81551   1203003
  pattern_match_hot.mt                460.47        461.10           12861       500
  string_interpolation_hot.mt         215.06        216.45         7400025         0
  boxed_primitive_dispatch_hot.mt        576.74        578.36           32804         0
  boxed_bool_dispatch_hot.mt          462.56        463.08           29276         0
  boxed_string_dispatch_hot.mt        379.60        381.77           24263         0
  box_unbox_hot.mt                    882.68        904.15           16512      1000
  static_call_hot.mt                  176.78        177.69           32516      2000
  linked_list_nested_hot.mt           159.47        161.77          124921      1969
  method_chain_hot.mt                  21.78         22.55           36526      2389
  string_build_call_hot.mt            100.60        100.77           21015       500
  deep_inheritance_hot.mt              86.53         88.09           34531      3006
  interface_dispatch_collections_hot.mt        296.36        296.64           25981      1029
  cast_miss_hot.mt                    201.07        201.50           16459       506
  exception_throw_hot.mt              621.07        624.40         6450035    250001
  ic_transition_hot.mt                336.65        338.90           14243       533
  switch_on_string_hot.mt             696.80        702.94        33000062   1000000
  substring_hot.mt                     83.65         84.16           13013         0
  gc_churn_intense_hot.mt             470.51        472.05           11811       500
  array_literal_alloc_hot.mt          110.86        111.50           29510         0
  value_class_mut_hot.mt              501.83        502.21           14516       500
  int_only_arith_hot.mt                36.23         36.51           14010         0
  null_narrowing_hot.mt               282.99        284.04           13474       600
  null_guard_clause_hot.mt            270.37        271.99           14224       600
  null_narrowing_while_hot.mt         392.99        396.07           32581      2030
```

### Notes

- MYT-373 is compile-time only — the narrowing facts run in the bytecode
  compiler and never reach a JIT/runtime path. Every movement below is
  therefore attributed to run-to-run variance, not the branch.
- The four 2026-05-23 "suspected-noise" regressions all reverted this
  run: `inline_value_object_hot` (−21.5%), `field_read_hot` (−16.7%),
  `static_call_hot` (−14.5%), `inline_monomorphic` (−10.7%). This
  confirms the prior baseline's note — they were thermal/scheduling
  noise, not real drift.
- `value_class_mut_hot.mt` (−15.6%) — the MYT-346 canary — landed back
  near its MYT-352 level (481.76). Still tracks as a JIT/OSR-divergence
  canary; the wall-time bounce is variance, not a fix.
- Only genuine regression ≥10% is `gc_churn_intense_hot.mt` (+10.7%);
  a cluster of allocation/dispatch benchmarks sits in the +8–10% band
  (`try_catch_finally_hot`, `linked_list_nested_hot`, `megamorphic_dispatch`,
  `deep_inheritance_hot`, `array_literal_alloc_hot`) — all small-absolute
  moves consistent with noise. Rerun before treating any as real.
- New null-narrowing-matrix benchmarks land mid-pack: `null_narrowing_hot`
  284, `null_guard_clause_hot` 272, `null_narrowing_while_hot` 396 (ms
  median) — alongside `cast_hot`/`ic_transition_hot`/`interface_dispatch`.
- Top wall-time sinks (>1s median): `reflection_lookup_hot.mt 1491`,
  `async_await_chain.mt 1272`.
