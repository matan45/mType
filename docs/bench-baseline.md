# mType Interpreter Baseline

Committed min wall-clock numbers from `mType.exe --benchmark`.
See `docs/benchmarks.md` for how to update.

Only the most recent run is kept here; prior runs live in git history
(`git log -p docs/bench-baseline.md`).

## 2026-05-23 — baseline refresh on branch MYT-361

- Machine: dev machine (Windows 11 Home)
- Commit: branch `MYT-361` (LSP-only fix — implements-check substitutes
  generic type parameters). No runtime/JIT changes since the prior
  2026-05-20 MYT-352 baseline; movements below reflect run-to-run
  variance plus any drift from intervening branch work.
- Build: Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)

### Notable movements vs prior 2026-05-20 (MYT-352) baseline

Regressions (median delta ≥ +10%):

| Benchmark | Prior | New | Δ |
| --- | ---: | ---: | ---: |
| `inline_value_object_hot.mt` | 162.25 | 215.39 | **+32.8%** |
| `value_class_mut_hot.mt` | 481.76 | 595.02 | **+23.5%** |
| `static_call_hot.mt` | 169.83 | 207.82 | **+22.4%** |
| `field_read_hot.mt` | 83.32 | 100.17 | **+20.2%** |
| `inline_monomorphic.mt` | 76.92 | 89.41 | **+16.2%** |

Improvements (median delta ≤ −5%):

| Benchmark | Prior | New | Δ |
| --- | ---: | ---: | ---: |
| `generic_dispatch_hot.mt` | 688.55 | 646.64 | **−6.1%** |
| `gc_churn_intense_hot.mt` | 449.40 | 426.44 | **−5.1%** |

### Summary (jit=on)

```
  Script                             min(ms)    median(ms)    instructions     calls
  arithmetic_tight_loop.mt            108.96        110.59           20017         0
  method_dispatch.mt                   99.25        101.14           14042       506
  object_alloc.mt                     351.72        352.35           13511         0
  object_alloc_nested.mt              737.07        737.65           17011       500
  gc_cycle_churn.mt                   246.01        247.82           26511    160900
  field_write_hot.mt                   79.37         80.04            8018         1
  field_read_hot.mt                    99.66        100.17            9020         1
  polymorphic_field_hot.mt            515.09        522.56        42000094         8
  string_ops.mt                       108.69        109.03           19019         0
  recursive.mt                        668.50        675.59           17260   2545487
  bitwise_tight_loop.mt                78.82         79.08           23019         0
  short_circuit_chain.mt               65.47         65.68           24909         0
  primitive_method_dispatch.mt        195.54        196.53           32041         0
  array_multi_alloc.mt                 36.28         36.47            9911       500
  array_multi_get.mt                  272.69        274.06           60117       500
  for_each_loop.mt                    113.19        114.22           54787      1007
  inline_monomorphic.mt                88.39         89.41           13016       501
  inline_branching.mt                  88.78         88.84           15016       501
  inline_polymorphic.mt                98.79         98.88           14051       508
  inline_polymorphic_mixed.mt         356.98        357.03           32926       508
  megamorphic_dispatch.mt              98.23         99.12           14069       512
  inline_value_object_hot.mt          213.45        215.39           12517       500
  function_call_hot.mt                 91.71         92.03           15011       500
  function_entry_tier_hot.mt           92.23         92.34           11811      1500
  function_call_medium_hot.mt         173.29        173.75           13911       500
  async_await_tight_loop.mt           627.90        628.00           12425       501
  async_await_chain.mt               1233.26       1239.47           23325      2001
  lambda_call_hot.mt                  691.44        702.40           12521   1999501
  lambda_closure_hot.mt               715.44        716.61           12526   1999502
  generic_dispatch_hot.mt             644.69        646.64           20074      1012
  try_catch_finally_hot.mt            342.83        345.39           50019      2000
  switch_dispatch_hot.mt              463.98        466.28           14634       500
  overload_dispatch_hot.mt            492.25        493.99           34026      2001
  abstract_dispatch_hot.mt             99.83        102.18           14042       506
  cast_hot.mt                         262.20        265.29           19560       505
  collections_hash_hot.mt             612.75        614.30           32763       502
  collections_hash_user_class_hot.mt        624.99        630.51           35775       502
  collections_hashset_hot.mt          155.17        156.31           18655         1
  stream_pipeline_hot.mt              375.26        377.33         2090493    292256
  reflection_lookup_hot.mt           1458.94       1473.34           81551   1203003
  pattern_match_hot.mt                456.75        458.55           12861       500
  string_interpolation_hot.mt         218.40        218.75         7400025         0
  boxed_primitive_dispatch_hot.mt        560.78        560.96           32805         0
  boxed_bool_dispatch_hot.mt          450.95        458.35           29276         0
  boxed_string_dispatch_hot.mt        374.97        375.57           24264         0
  box_unbox_hot.mt                    889.67        905.97           16513      1000
  static_call_hot.mt                  204.70        207.82           32516      2000
  linked_list_nested_hot.mt           146.80        147.38          124921      1969
  method_chain_hot.mt                  20.79         20.97           36526      2389
  string_build_call_hot.mt             96.67         96.71           21015       500
  deep_inheritance_hot.mt              80.85         80.95           34531      3006
  interface_dispatch_collections_hot.mt        285.86        286.21           25981      1029
  cast_miss_hot.mt                    190.57        190.72           16459       506
  exception_throw_hot.mt              622.14        624.36         6450035    250001
  ic_transition_hot.mt                333.02        334.88           14243       533
  switch_on_string_hot.mt             699.35        699.96        33000062   1000000
  substring_hot.mt                     83.46         84.27           13013         0
  gc_churn_intense_hot.mt             423.30        426.44           11811       500
  array_literal_alloc_hot.mt          102.58        102.73           29510         0
  value_class_mut_hot.mt              588.84        595.02           14516       500
  int_only_arith_hot.mt                38.28         38.42           14010         0
```

### Notes

- `value_class_mut_hot.mt` (+23.5%) is the MYT-346 canary — needs
  follow-up to confirm whether this is run-to-run noise or whether
  intervening branch work past 2026-05-20 has drifted the value-class
  JIT/OSR path away from interpreter parity again. Cross-check against
  a `--no-jit` reference run.
- `inline_value_object_hot.mt` (+32.8%) and `inline_monomorphic.mt`
  (+16.2%) move together — both touch the JIT inline path that MYT-352
  reshuffled; likely candidates for the same root cause.
- `static_call_hot.mt` (+22.4%) and `field_read_hot.mt` (+20.2%) are
  small-absolute benchmarks where double-digit % deltas can come from
  thermal/scheduling noise alone; rerun once before treating as real.
- Headline `object_alloc_nested.mt` from the MYT-352 baseline (752.59 →
  737.65 median, −2.0%) holds — the inlining lift is still landing.
- Top wall-time sinks (>1s median): `reflection_lookup_hot.mt 1473`,
  `async_await_chain.mt 1239`.
