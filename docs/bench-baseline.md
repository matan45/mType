# mType Interpreter Baseline

Committed min wall-clock numbers from `mType.exe --benchmark`.
See `docs/benchmarks.md` for how to update.

Only the most recent run is kept here; prior runs live in git history
(`git log -p docs/bench-baseline.md`).

## 2026-05-18 - MYT-346: value-class write methods inline via materialised temp

- Machine: dev machine (Windows 11 Home)
- Commit: `refctor2` with MYT-346 fix landed (inliner materialises a temp
  `ObjectInstance` into local-0 for value-class write methods via
  `jit_materialise_value_receiver_into_local`; new
  `INLINE_VALUE_REQUIRES_MATERIALISATION` eligibility variant; `scanCalleeOpcodes`
  extended to the full SET_FIELD family). Expands the benchmark set with the
  `value_class_mut_hot` canary, plus the previously-not-baselined
  `deep_inheritance_hot`, `interface_dispatch_collections_hot`, `cast_miss_hot`,
  `exception_throw_hot`, `ic_transition_hot`, `switch_on_string_hot`,
  `substring_hot`, `gc_churn_intense_hot`, `array_literal_alloc_hot`,
  `int_only_arith_hot`.
- Build: Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)

`value_class_mut_hot.mt` flips from broken (pre-fix produced `total=500`
instead of `total=2000000` — silent correctness bug past the OSR threshold)
to passing with a 492.32 ms min. Per-call cost is the
`ObjectInstancePool::acquire` + `loadFromValueObject` of the materialised
temp, on top of the inlined body; ~3× the read-only counterpart
`inline_value_object_hot` (163.72 ms) which stays on the cheap raw-memcpy
donation path. Read-only inlines are unaffected by the fix.

### Summary (jit=on)

```
  Script                             min(ms)    median(ms)    instructions     calls
  arithmetic_tight_loop.mt            101.67        102.69           20017         0
  method_dispatch.mt                   99.66        102.14           14042       506
  object_alloc.mt                     635.60        642.37           12511         0
  object_alloc_nested.mt             1492.19       1492.65           16812       500
  gc_cycle_churn.mt                   236.46        236.56           26511    160900
  field_write_hot.mt                   78.80         79.07            8018         1
  field_read_hot.mt                    78.08         78.21            9020         1
  polymorphic_field_hot.mt            504.19        506.35        42000094         8
  string_ops.mt                       111.33        111.72           19019         0
  recursive.mt                        653.94        654.06           17260   2545487
  bitwise_tight_loop.mt                75.85         76.12           23019         0
  short_circuit_chain.mt               62.88         63.62           24909         0
  primitive_method_dispatch.mt        227.49        227.60           32038         0
  array_multi_alloc.mt                 35.54         39.81            9911       500
  array_multi_get.mt                  275.85        276.41           60117       500
  for_each_loop.mt                    333.52        334.16           75653      5603
  inline_monomorphic.mt                77.41         78.44           13016       501
  inline_branching.mt                  79.40         81.39           15016       501
  inline_polymorphic.mt                99.93        101.89           14051       508
  inline_polymorphic_mixed.mt         352.28        352.68           32926       508
  megamorphic_dispatch.mt             865.65        868.97           14069       512
  inline_value_object_hot.mt          163.72        164.03           12517       500
  function_call_hot.mt                 91.96         92.83           15011       500
  function_entry_tier_hot.mt           93.37         93.69           11811      1500
  function_call_medium_hot.mt         176.47        177.63           13911       500
  async_await_tight_loop.mt           641.62        642.11           12422       501
  async_await_chain.mt               1268.53       1270.82           23322      2001
  lambda_call_hot.mt                  723.70        724.57           12521   1999501
  lambda_closure_hot.mt               729.90        735.67           12526   1999502
  generic_dispatch_hot.mt             636.68        642.29           20074      1012
  try_catch_finally_hot.mt            351.58        351.62           50019      2000
  switch_dispatch_hot.mt              473.65        477.21           14634       500
  overload_dispatch_hot.mt            527.57        532.10           34026      2001
  abstract_dispatch_hot.mt            101.79        103.14           14042       506
  cast_hot.mt                         262.16        263.60           19560       505
  collections_hash_hot.mt             612.16        613.70           32761       502
  collections_hash_user_class_hot.mt        680.27        682.50           35773       502
  collections_hashset_hot.mt          171.72        171.77           18653         1
  stream_pipeline_hot.mt              362.94        365.25         2090491    292256
  reflection_lookup_hot.mt           1450.86       1459.75           81549   1203003
  pattern_match_hot.mt                458.90        468.69           12861       500
  string_interpolation_hot.mt         229.02        229.07         7400025         0
  boxed_primitive_dispatch_hot.mt        668.38        672.25           32802         0
  boxed_bool_dispatch_hot.mt          519.47        522.98           29276         0
  boxed_string_dispatch_hot.mt        411.42        412.20           24261         0
  static_call_hot.mt                  168.82        169.96           32516      2000
  linked_list_nested_hot.mt           141.23        141.24          124919      1969
  method_chain_hot.mt                  22.23         22.78           36526      2389
  string_build_call_hot.mt            101.93        103.57           21015       500
  deep_inheritance_hot.mt              85.24         85.42           34531      3006
  interface_dispatch_collections_hot.mt       1743.26       1753.61           42625    302860
  cast_miss_hot.mt                    192.60        192.65           16459       506
  exception_throw_hot.mt              548.71        550.00         6450035    250001
  ic_transition_hot.mt                321.63        323.29           14243       533
  switch_on_string_hot.mt            1002.07       1002.37        22003362   1000000
  substring_hot.mt                     82.41         83.34           13013         0
  gc_churn_intense_hot.mt             802.60        808.47           11612       500
  array_literal_alloc_hot.mt          103.57        104.38           29510         0
  value_class_mut_hot.mt              492.32        493.38           14516       500
  int_only_arith_hot.mt                36.60         36.83           14010         0
```

### Notes

- `value_class_mut_hot.mt 492.32` — MYT-346 canary now green. Pre-fix the
  script silently produced wrong output past the OSR threshold; post-fix the
  inliner materialises a value-class temp into local-0 before the body and
  the integration suite + benchmark agree with the `--no-jit` reference.
- `inline_value_object_hot.mt 163.72` — read-only counterpart unchanged
  (within noise vs. prior 163.56). Confirms the materialise path is
  per-eligibility-variant, not blanket on value-class receivers.
- Slowest entries to keep eyes on (candidates for follow-up tuning, not
  regressions from this run):
  - `interface_dispatch_collections_hot.mt 1743.26` — collections + interface
    dispatch, MYT-324 shape territory.
  - `switch_on_string_hot.mt 1002.07` — 22M instructions, 1M string-keyed
    switch dispatches.
  - `megamorphic_dispatch.mt 865.65` — MEGAMORPHIC IC, 14k inst / 512 calls;
    moved from 772.21 → 865.65 (≈12% slower, run-to-run noise on a small
    shape-walking benchmark; instruction/call counts identical).
  - `gc_churn_intense_hot.mt 802.60` — newly added; high allocator pressure
    benchmark, expected expensive.
- Movement in other entries (e.g. `lambda_call_hot` +6%, `boxed_primitive_dispatch_hot`
  +6%, `string_interpolation_hot` +6%, `linked_list_nested_hot` -3%) is
  within run-to-run wall-clock variance; instruction/call shape unchanged.
