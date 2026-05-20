# mType Interpreter Baseline

Committed min wall-clock numbers from `mType.exe --benchmark`.
See `docs/benchmarks.md` for how to update.

Only the most recent run is kept here; prior runs live in git history
(`git log -p docs/bench-baseline.md`).

## 2026-05-20 - add box_unbox_hot benchmark

- Machine: dev machine (Windows 11 Home)
- Commit: branch `refctor` (post-refactor; first measured run that includes
  the new `box_unbox_hot.mt` micro covering the `JitCompiler_Boxing.cpp`
  emit surface — `Box<Int>(new Int(i))` chase per iteration to isolate
  box/unbox pressure from the bundled `boxed_*_dispatch_hot` benchmarks).
- Build: Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)

### New entry

`box_unbox_hot.mt` at median **1449.22 ms** — currently the third-slowest
benchmark in the canonical suite (after `object_alloc_nested` 1593 and
`reflection_lookup_hot` 1533). Two heap allocations per iteration (Box +
Int) plus a two-step unwrap (`b.getValue().getValue()`) dominate. Next
steps: `--jit-stats` + `--profile=full` to confirm whether `jit_box_int` /
`jit_unbox_*` helper calls are the long pole vs the allocation/GC surface.

### Summary (jit=on)

```
  Script                             min(ms)    median(ms)    instructions     calls
  arithmetic_tight_loop.mt            104.09        104.10           20017         0
  method_dispatch.mt                  104.23        105.33           14042       506
  object_alloc.mt                     631.13        638.40           12511         0
  object_alloc_nested.mt             1588.95       1593.74           16812       500
  gc_cycle_churn.mt                   254.91        256.73           26511    160900
  field_write_hot.mt                   80.40         80.91            8018         1
  field_read_hot.mt                    81.13         81.28            9020         1
  polymorphic_field_hot.mt            537.91        546.29        42000094         8
  string_ops.mt                       113.47        113.65           19019         0
  recursive.mt                        693.77        696.17           17260   2545487
  bitwise_tight_loop.mt                78.69         79.31           23019         0
  short_circuit_chain.mt               63.58         63.96           24909         0
  primitive_method_dispatch.mt        224.64        226.66           32038         0
  array_multi_alloc.mt                 36.05         36.09            9911       500
  array_multi_get.mt                  281.83        282.12           60117       500
  for_each_loop.mt                    107.87        108.39           54785      1007
  inline_monomorphic.mt                81.34         82.01           13016       501
  inline_branching.mt                  84.81         84.85           15016       501
  inline_polymorphic.mt               105.58        106.11           14051       508
  inline_polymorphic_mixed.mt         373.05        373.49           32926       508
  megamorphic_dispatch.mt             104.90        105.08           14069       512
  inline_value_object_hot.mt          161.84        165.68           12517       500
  function_call_hot.mt                 92.87         93.40           15011       500
  function_entry_tier_hot.mt           93.31         93.69           11811      1500
  function_call_medium_hot.mt         185.75        186.02           13911       500
  async_await_tight_loop.mt           659.94        665.16           12422       501
  async_await_chain.mt               1296.08       1296.31           23322      2001
  lambda_call_hot.mt                  662.62        664.44           12521   1999501
  lambda_closure_hot.mt               676.29        677.96           12526   1999502
  generic_dispatch_hot.mt             660.37        662.85           20074      1012
  try_catch_finally_hot.mt            360.92        361.27           50019      2000
  switch_dispatch_hot.mt              491.62        500.27           14634       500
  overload_dispatch_hot.mt            505.18        505.92           34026      2001
  abstract_dispatch_hot.mt            105.83        106.54           14042       506
  cast_hot.mt                         276.87        279.41           19560       505
  collections_hash_hot.mt             620.98        621.87           32761       502
  collections_hash_user_class_hot.mt        699.69        718.80           35773       502
  collections_hashset_hot.mt          172.76        174.45           18653         1
  stream_pipeline_hot.mt              378.11        379.19         2090491    292256
  reflection_lookup_hot.mt           1528.74       1533.95           81549   1203003
  pattern_match_hot.mt                479.34        479.55           12861       500
  string_interpolation_hot.mt         219.40        220.48         7400025         0
  boxed_primitive_dispatch_hot.mt        615.77        616.27           32802         0
  boxed_bool_dispatch_hot.mt          517.97        518.01           29276         0
  boxed_string_dispatch_hot.mt        395.43        397.73           24261         0
  box_unbox_hot.mt                   1444.77       1449.22           16511      1000
  static_call_hot.mt                  166.77        167.29           32516      2000
  linked_list_nested_hot.mt           151.14        151.58          124919      1969
  method_chain_hot.mt                  22.09         22.26           36526      2389
  string_build_call_hot.mt             98.07         98.44           21015       500
  deep_inheritance_hot.mt              86.72         87.92           34531      3006
  interface_dispatch_collections_hot.mt        297.60        298.49           25979      1029
  cast_miss_hot.mt                    201.36        202.10           16459       506
  exception_throw_hot.mt              615.05        616.44         6450035    250001
  ic_transition_hot.mt                331.58        333.96           14243       533
  switch_on_string_hot.mt             698.04        700.01        33000062   1000000
  substring_hot.mt                     81.99         82.37           13013         0
  gc_churn_intense_hot.mt             813.31        819.70           11612       500
  array_literal_alloc_hot.mt          104.47        104.88           29510         0
  value_class_mut_hot.mt              482.50        485.00           14516       500
  int_only_arith_hot.mt                35.07         35.73           14010         0
```

### Notes

- `value_class_mut_hot.mt` continues to agree with the `--no-jit` reference
  post-MYT-346; the canary stays green.
- New `box_unbox_hot.mt` is the largest single-row addition; investigation
  pending via `--jit-stats` + `--profile=full` to attribute cost between
  alloc/GC pressure and the `jit_box_*` / `jit_unbox_*` `cc.invoke` helpers
  (see `mType/vm/jit/JitCompiler_Boxing.cpp`).
- A handful of pre-existing slow paths move 3–10% relative to the
  2026-05-18 baseline (`exception_throw_hot` 552→616, `switch_dispatch_hot`
  468→500, `gc_cycle_churn` 242→256). These are within the post-refactor
  noise band on a single 3-iteration run; rerun with `perf-compare.ps1
  -Iterations 7` if any becomes a candidate for a regression ticket.
- Top wall-time sinks (>1s median) remain:
  - `object_alloc_nested.mt 1593` — `Point` field-init walk in `distanceSq`.
  - `reflection_lookup_hot.mt 1533` — `new Field` / `new Method` wrapper
    allocation in `Class.mt`.
  - `box_unbox_hot.mt 1449` — new entry, see above.
  - `async_await_chain.mt 1296` — `cc.invoke` frame setup on the AWAIT path.
