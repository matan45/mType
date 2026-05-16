# mType Interpreter Baseline

Committed min wall-clock numbers from `mType.exe --benchmark`.
See `docs/benchmarks.md` for how to update.

Only the most recent run is kept here; prior runs live in git history
(`git log -p docs/bench-baseline.md`).

## 2026-05-16 - MYT-324 (JIT combat-tick OSR guard refinement)

- Machine: dev machine (Windows 11 Home)
- Commit: branch with MYT-324 fix on top of `dev`
- Build: Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)

MYT-324 adds a regression for the combat-tick registry loop crash and narrows
the OSR rejection guard to the unsafe forward-conditional helper-call shapes.
The affected benchmark canaries (`cast_hot`, `collections_hash_hot`,
`collections_hash_user_class_hot`, `linked_list_nested_hot`,
`stream_pipeline_hot`, and `method_chain_hot`) retain their expected
instruction/call-count shape after the guard refinement.

### Summary (jit=on)

```
  Script                             min(ms)    median(ms)    instructions     calls
  arithmetic_tight_loop.mt            106.39        107.25           20017         0
  method_dispatch.mt                  103.56        105.70           14042       506
  object_alloc.mt                     631.32        642.21           12511         0
  object_alloc_nested.mt             1533.25       1536.63           16811       500
  gc_cycle_churn.mt                   245.62        245.86           26511    160900
  field_write_hot.mt                   76.76         77.92            8018         1
  field_read_hot.mt                    77.27         77.33            9020         1
  polymorphic_field_hot.mt            552.68        553.65        42000094         8
  string_ops.mt                       111.03        111.16           19019         0
  recursive.mt                        689.71        689.94           17260   2545487
  bitwise_tight_loop.mt                78.12         79.80           23019         0
  short_circuit_chain.mt               64.50         65.17           24909         0
  primitive_method_dispatch.mt        221.88        224.37           32041         0
  array_multi_alloc.mt                 35.64         35.98            9911       500
  array_multi_get.mt                  277.06        279.12           60117       500
  for_each_loop.mt                    350.25        352.14           75655      5603
  inline_monomorphic.mt                79.82         80.35           13016       501
  inline_branching.mt                  82.69         84.39           15016       501
  inline_polymorphic.mt               103.82        104.33           14051       508
  inline_polymorphic_mixed.mt         363.58        364.69           32926       508
  megamorphic_dispatch.mt             780.23        785.47           14069       512
  inline_value_object_hot.mt          166.95        167.00           12517       500
  function_call_hot.mt                 94.52         94.64           15011       500
  function_entry_tier_hot.mt           94.04         95.12           11811      1500
  function_call_medium_hot.mt         184.71        185.57           13911       500
  async_await_tight_loop.mt           644.60        649.24           12425       501
  async_await_chain.mt               1298.09       1301.09           23325      2001
  lambda_call_hot.mt                  667.18        669.68           12521   1999501
  lambda_closure_hot.mt               690.18        701.14           12526   1999502
  generic_dispatch_hot.mt             659.97        660.59           20074      1012
  try_catch_finally_hot.mt            355.40        357.95           50019      2000
  switch_dispatch_hot.mt              491.23        495.34           14634       500
  overload_dispatch_hot.mt            493.87        496.42           34026      2001
  abstract_dispatch_hot.mt            104.73        106.97           14042       506
  cast_hot.mt                         270.16        271.25           19560       505
  collections_hash_hot.mt             645.07        646.52           32763       502
  collections_hash_user_class_hot.mt        710.86        742.05           35775       502
  collections_hashset_hot.mt          177.35        183.24           18655         1
  stream_pipeline_hot.mt              388.31        395.65         2090493    292256
  reflection_lookup_hot.mt           1489.02       1493.35           81551   1203003
  pattern_match_hot.mt                484.72        487.41           12861       500
  string_interpolation_hot.mt         216.24        219.55         7400025         0
  boxed_primitive_dispatch_hot.mt        622.82        623.30           32805         0
  boxed_bool_dispatch_hot.mt          512.97        516.63           29276         0
  boxed_string_dispatch_hot.mt        390.58        390.88           24264         0
  static_call_hot.mt                  169.94        169.95           32516      2000
  linked_list_nested_hot.mt           150.89        153.76          124921      1969
  method_chain_hot.mt                  20.86         21.03           36526      2389
  string_build_call_hot.mt             98.03         98.06           21015       500
```

### Notes

- `cast_hot.mt`, `collections_hash_hot.mt`, and
  `collections_hash_user_class_hot.mt` are back to baseline instruction/call
  counts after narrowing the MYT-324 OSR guard.
- `megamorphic_dispatch.mt` remains flat-to-faster versus the previous
  baseline and was not a regression.
- Timing movement outside those shape changes is treated as normal
  run-to-run wall-clock variance unless instruction/call counts change.
