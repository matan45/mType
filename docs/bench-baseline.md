# mType Interpreter Baseline

Committed min wall-clock numbers from `mType.exe --benchmark`.
See `docs/benchmarks.md` for how to update.

Only the most recent run is kept here; prior runs live in git history
(`git log -p docs/bench-baseline.md`).

## 2026-05-15 — MYT-314 (function-entry JIT tier-up wired + framePrg fix)

- Machine: dev machine (Windows 11 Home)
- Commit:  `a0a13bb0` (branch `MYT-314`)
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)

MYT-314 wires `PROFILE_ENTER` → `JitProfiler::recordEntry` → `JitCompiler::
compile` so hot non-looping functions tier up via the function-entry path
(not only via OSR). This run also folds in the `framePrg` argument fix at
`VirtualMachineDispatch.cpp:844` so imported hot functions no longer
silently bail out in `JitCompiler::compile`. New benchmark
`function_entry_tier_hot.mt` lands as the dedicated acceptance signal —
three small leaves with no internal loop, each called 2M times. With the
plain-function inliner still disabled, the only path to native code for
those leaves is MYT-314's tier-up.

### Summary (jit=on)

```
  Script                             min(ms)    median(ms)    instructions     calls
  arithmetic_tight_loop.mt            110.85        110.98           20017         0
  method_dispatch.mt                  103.68        106.50           14042       506
  object_alloc.mt                     676.42        681.65           12511         0
  object_alloc_nested.mt             1517.58       1524.48           16811       500
  field_write_hot.mt                   79.44         80.48            8018         1
  field_read_hot.mt                    80.58         81.20            9020         1
  polymorphic_field_hot.mt            502.54        506.54        42000094         8
  string_ops.mt                       132.51        133.32           19019         0
  recursive.mt                        656.52        656.62           17260   2545487
  bitwise_tight_loop.mt                80.56         81.42           23019         0
  short_circuit_chain.mt               66.35         66.41           24909         0
  primitive_method_dispatch.mt        247.64        249.23           32041         0
  array_multi_alloc.mt                 74.98         77.89            9911       500
  array_multi_get.mt                  348.74        351.21           60117       500
  for_each_loop.mt                    352.78        353.10           75655      5604
  inline_monomorphic.mt                79.23         79.68           13016       501
  inline_branching.mt                  82.27         84.10           15016       501
  inline_polymorphic.mt               104.50        105.74           14051       508
  inline_polymorphic_mixed.mt        1101.75       1106.59           32926       508
  megamorphic_dispatch.mt             824.73        828.24           14069       512
  inline_value_object_hot.mt          143.08        145.14           12517       500
  function_call_hot.mt                184.98        185.61           15011       500
  function_entry_tier_hot.mt          475.51        475.79           11811      1500
  async_await_tight_loop.mt           680.62        681.31           12425       501
  async_await_chain.mt               1339.00       1341.45           23325      2001
  lambda_call_hot.mt                  802.59        808.26           12521   1999501
  lambda_closure_hot.mt               836.20        838.01           12526   1999502
  generic_dispatch_hot.mt             641.48        646.38           20074      1012
  try_catch_finally_hot.mt            337.76        338.48           50019      2000
  switch_dispatch_hot.mt              455.08        459.28           14634       500
  overload_dispatch_hot.mt            560.43        569.80           34026      2001
  abstract_dispatch_hot.mt            102.20        104.93           14042       506
  cast_hot.mt                         269.12        270.78           19560       505
  collections_hash_hot.mt             637.84        647.12           32763       502
  collections_hash_user_class_hot.mt        711.06        716.05           35775       502
  collections_hashset_hot.mt          181.16        181.17           18655         1
  stream_pipeline_hot.mt              402.70        402.89         2090493    306881
  reflection_lookup_hot.mt           2039.04       2042.81           81551   1203003
  pattern_match_hot.mt                446.78        449.79           12861       500
  string_interpolation_hot.mt         218.69        220.12         7400025         0
  boxed_primitive_dispatch_hot.mt        660.84        662.77           32805         0
  boxed_bool_dispatch_hot.mt          537.10        539.61           29276         0
  boxed_string_dispatch_hot.mt        402.83        403.12           24264         0
  static_call_hot.mt                  144.27        144.50           32516      2000
  linked_list_nested_hot.mt           312.79        313.46          124921     81001
  method_chain_hot.mt                  21.36         21.83           36526      2389
  string_build_call_hot.mt             98.56         99.11           21015       500
```

### Notes on the new entry and deltas vs the prior run (2026-05-14, in git history)

- **`function_entry_tier_hot.mt` at 475.51 ms** (new) — three small leaves
  (`add3`, `mul2`, `combine`) each called 2M times from the outer loop.
  `total=6000003000000` matches `--no-jit` reference. This number is the
  acceptance baseline for MYT-314's tier-up path; speedups would come
  later from MYT-132 JIT→JIT direct dispatch into these leaves and from
  re-enabling the plain-function inliner. Per-iter cost (~238 ns for 3
  leaf calls + 1 outer add + 1 store) is in the same ballpark as
  `function_call_hot.mt` (~93 ns per iter for 1 distanceSq call) scaled by
  call count.
- **`recursive.mt`: 771.72 → 656.52 ms (−14.9%)** — the most-improved
  pre-existing benchmark. fib/ack/gcd are exactly the function-entry tier
  target (recursive leaves, no inner loop), so PROFILE_ENTER now compiles
  them where OSR could not. Confirms the wiring is doing real work outside
  the synthetic test.
- **`inline_polymorphic_mixed.mt`: 1560.58 → 1101.75 ms (−29.4%)** — Big's
  oversized per-shape helper body now JIT-compiles via PROFILE_ENTER once
  its 500k calls cross the threshold, so the non-inlined shape stops
  paying interpreter dispatch on the dominant path.
- **`megamorphic_dispatch.mt`: 860.04 → 824.73 ms (−4.1%)** — small win
  from each shape's body becoming native after tier-up; the dispatcher
  itself is unchanged.
- **`try_catch_finally_hot.mt`: 441.82 → 337.76 ms (−23.5%)** — handler
  bodies are short non-looping leaves; same tier-up story.
- Everything else is within ±3% of 2026-05-14, i.e. noise.

### Sanity outputs (must match on re-run for same commit)

- `function_entry_tier_hot.mt`: `function_entry_tier_hot total=6000003000000`
- `myt314JitFunctionEntryTier_pass.mt` (integration fixture): `myt314 total=11325`
