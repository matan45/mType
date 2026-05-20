# mType Interpreter Baseline

Committed min wall-clock numbers from `mType.exe --benchmark`.
See `docs/benchmarks.md` for how to update.

Only the most recent run is kept here; prior runs live in git history
(`git log -p docs/bench-baseline.md`).

## 2026-05-20 - primitive-wrapper resolution cache (provisional)

- Machine: dev machine (Windows 11 Home)
- Commit: branch `benchmark`. Adds `tryCreatePrimitiveValueObjectCached`
  in `BoxingUtils.hpp` + a `mutable std::vector<PrimitiveWrapperResolution>`
  side-table on `BytecodeProgram` keyed by constant-pool string index
  (classIndex). First call at each classIndex resolves
  (className → ClassDefinition*, PrimitiveTypeTag, valueFieldAtIndex0) and
  caches; subsequent calls skip `classNameToPrimitiveTag` + ClassRegistry
  `findClass` hashmap + `getFieldIndex("value")` hashmap. Wired into
  `jit_new_value_object`.
- Build: Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)

### Headline movements vs 2026-05-20 pre-patch baseline

| Benchmark | Prior | New | Δ | Driver |
| --- | ---: | ---: | ---: | --- |
| `primitive_method_dispatch.mt` | 226.66 | 191.75 | **−15.4%** | Int/Float/Bool wrapper alloc per iter |
| `boxed_bool_dispatch_hot.mt` | 518.01 | 453.21 | **−12.5%** | new Bool(...) hot loop |
| `collections_hashset_hot.mt` | 174.45 | 158.27 | **−9.3%** | implicit wrapper allocation on hash path |
| `boxed_primitive_dispatch_hot.mt` | 616.27 | 570.29 | **−7.5%** | mixed Int/Float/Bool/String wrappers |
| `boxed_string_dispatch_hot.mt` | 397.73 | 377.52 | **−5.1%** | new String(...) per iter |
| `box_unbox_hot.mt` | 1449.22 | 1395.66 | **−3.7%** | target benchmark (Box<Int>(new Int(i))) |

The cache lands the expected savings on every primitive-wrapper allocation
hot path: ~40ns/alloc removed (4 string compares + 2 hashmaps + 1 field-index
lookup). Larger wins concentrate where the wrapper alloc is the *only*
significant per-iter work (`primitive_method_dispatch`, `boxed_bool`); the
mixed benchmarks show smaller relative wins because other costs dilute.

### Provisional regressions (need statistical re-validation)

Several **unrelated** benchmarks regressed 13-23% in this run:

| Benchmark | Prior | New | Δ |
| --- | ---: | ---: | ---: |
| `lambda_closure_hot.mt` | 677.96 | 832.60 | +22.8% |
| `lambda_call_hot.mt` | 664.44 | 793.09 | +19.4% |
| `function_call_medium_hot.mt` | 186.02 | 220.45 | +18.5% |
| `try_catch_finally_hot.mt` | 361.27 | 420.92 | +16.5% |
| `inline_polymorphic_mixed.mt` | 373.49 | 432.09 | +15.7% |
| `generic_dispatch_hot.mt` | 662.85 | 752.73 | +13.6% |
| `pattern_match_hot.mt` | 479.55 | 543.27 | +13.3% |
| `switch_dispatch_hot.mt` | 500.27 | 564.47 | +12.8% |
| `object_alloc_nested.mt` | 1593.74 | 1710.30 | +7.3% |

The patch touches only `jit_new_value_object` (primitive-wrapper alloc) —
none of these regressions have a plausible mechanism to come from this
change. Most likely **single-3-iteration variance + thermal drift** across
a ~60s suite run. Before landing the cache patch, re-run the suspect
regressions with `perf-compare.ps1 -Mode compare -Iterations 7+` to
distinguish noise from real cross-effects. If any persists with p<0.05,
investigate before merging.

### Summary (jit=on)

```
  Script                             min(ms)    median(ms)    instructions     calls
  arithmetic_tight_loop.mt            109.65        109.78           20017         0
  method_dispatch.mt                  102.48        102.49           14042       506
  object_alloc.mt                     621.51        623.95           12511         0
  object_alloc_nested.mt             1702.12       1710.30           16811       500
  gc_cycle_churn.mt                   248.36        249.08           26511    160900
  field_write_hot.mt                   76.19         77.39            8018         1
  field_read_hot.mt                    80.00         81.64            9020         1
  polymorphic_field_hot.mt            525.44        525.82        42000094         8
  string_ops.mt                       108.01        110.71           19019         0
  recursive.mt                        675.94        678.18           17260   2545487
  bitwise_tight_loop.mt                79.52         80.50           23019         0
  short_circuit_chain.mt               64.60         64.92           24909         0
  primitive_method_dispatch.mt        191.05        191.75           32038         0
  array_multi_alloc.mt                 36.61         36.74            9911       500
  array_multi_get.mt                  275.79        276.97           60117       500
  for_each_loop.mt                    104.42        106.50           54785      1007
  inline_monomorphic.mt                79.24         79.78           13016       501
  inline_branching.mt                  81.17         81.36           15016       501
  inline_polymorphic.mt               102.89        104.02           14051       508
  inline_polymorphic_mixed.mt         427.22        432.09           32926       508
  megamorphic_dispatch.mt             102.00        102.28           14069       512
  inline_value_object_hot.mt          165.82        166.32           12517       500
  function_call_hot.mt                 94.72         95.47           15011       500
  function_entry_tier_hot.mt           94.89         96.40           11811      1500
  function_call_medium_hot.mt         216.47        220.45           13911       500
  async_await_tight_loop.mt           625.62        626.52           12422       501
  async_await_chain.mt               1291.75       1301.27           23322      2001
  lambda_call_hot.mt                  788.01        793.09           12521   1999501
  lambda_closure_hot.mt               826.06        832.60           12526   1999502
  generic_dispatch_hot.mt             750.52        752.73           20074      1012
  try_catch_finally_hot.mt            419.85        420.92           50019      2000
  switch_dispatch_hot.mt              559.44        564.47           14634       500
  overload_dispatch_hot.mt            506.31        520.61           34026      2001
  abstract_dispatch_hot.mt            103.12        104.60           14042       506
  cast_hot.mt                         269.42        272.47           19560       505
  collections_hash_hot.mt             630.48        643.22           32761       502
  collections_hash_user_class_hot.mt        712.67        713.53           35773       502
  collections_hashset_hot.mt          157.05        158.27           18653         1
  stream_pipeline_hot.mt              380.32        382.66         2090491    292256
  reflection_lookup_hot.mt           1533.91       1540.76           81549   1203003
  pattern_match_hot.mt                543.04        543.27           12861       500
  string_interpolation_hot.mt         213.30        214.87         7400025         0
  boxed_primitive_dispatch_hot.mt        565.46        570.29           32802         0
  boxed_bool_dispatch_hot.mt          453.15        453.21           29276         0
  boxed_string_dispatch_hot.mt        373.49        377.52           24261         0
  box_unbox_hot.mt                   1393.49       1395.66           16511      1000
  static_call_hot.mt                  168.22        168.38           32516      2000
  linked_list_nested_hot.mt           152.12        152.30          124919      1969
  method_chain_hot.mt                  21.03         21.51           36526      2389
  string_build_call_hot.mt            107.51        107.76           21015       500
  deep_inheritance_hot.mt              84.98         85.00           34531      3006
  interface_dispatch_collections_hot.mt        305.47        309.07           25979      1029
  cast_miss_hot.mt                    197.02        197.22           16459       506
  exception_throw_hot.mt              615.55        616.07         6450035    250001
  ic_transition_hot.mt                345.74        348.84           14243       533
  switch_on_string_hot.mt             694.73        696.20        33000062   1000000
  substring_hot.mt                     82.47         82.81           13013         0
  gc_churn_intense_hot.mt             848.33        851.57           11611       500
  array_literal_alloc_hot.mt          104.56        104.74           29510         0
  value_class_mut_hot.mt              487.81        488.63           14516       500
  int_only_arith_hot.mt                39.89         40.04           14010         0
```

### Notes

- Primitive-wrapper allocation cache lands the predicted shape of win:
  benchmarks dominated by `new Int/Float/Bool/String(...)` in a hot loop
  pick up 5-15%. `value_class_mut_hot.mt` continues to match the
  `--no-jit` reference; MYT-346 canary stays green.
- **Provisional regressions need re-validation** before this patch
  lands. The bench-baseline.md format intentionally records the raw
  3-iteration run; treat the unrelated-benchmark regressions as
  suspect-not-confirmed until `perf-compare.ps1 -Iterations 7` resolves
  them.
- Top wall-time sinks (>1s median):
  - `object_alloc_nested.mt 1710` — provisional regression of +7% over
    prior baseline (was 1593). The `Point` constructor inside
    `distanceSq` allocates two heap objects per iter that this patch
    does NOT touch; needs re-validation under perf-compare to
    distinguish noise from real cross-effect.
  - `reflection_lookup_hot.mt 1540` — flat vs prior (1533).
  - `box_unbox_hot.mt 1395` — improved from 1449 (target win).
  - `async_await_chain.mt 1301` — flat vs prior (1296).
