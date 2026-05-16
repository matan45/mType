# mType Interpreter Baseline

Committed min wall-clock numbers from `mType.exe --benchmark`.
See `docs/benchmarks.md` for how to update.

Only the most recent run is kept here; prior runs live in git history
(`git log -p docs/bench-baseline.md`).

## 2026-05-15 — MYT-322 (free-function direct JIT dispatch: pre-cached IC fields + size gate + lazy refresh)

- Machine: dev machine (Windows 11 Home)
- Commit:  branch `MYT-322` (on top of `dev`)
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)

MYT-322 re-lands the free-function side of direct JIT-to-JIT dispatch that
MYT-321's first attempt had to revert. The two gates that prevent the
+56% `generic_dispatch_hot.mt` regression:

1. **Pre-cached IC fields** (`CachedInstructionState::cachedJitFnPtr` +
   `cachedFrameName` populated at `jit_call_function_ic` cold-fill,
   `cachedProgramIndex` bug fix from unconditional `0`). The helper
   `jit_call_function_direct` takes the pre-interned `FunctionNameHandle`
   and resolved `programIndex` as parameters, skipping the per-call
   `internFrameName` hashmap probe + `loadedPrograms` scan.
2. **Callee-size gate** (`MIN_DIRECT_CALL_INSTRUCTION_COUNT = 15` in
   `JitHelpers.hpp`). Tiny callees (the original regression cause) stay
   on `callFunctionFromJitDirect`'s mini-interpret path; only callees
   above the threshold pay the `cc.new_stack` prologue cost in exchange
   for the direct dispatch.
3. **Invalidation wiring**: new `BytecodeProgram::clearCachedJitFnPtrFor`
   scrubs IC slots on every loaded program when
   `VirtualMachine::invalidateInlinedFunctionCallers` frees native code,
   matching the method-side `InlineCacheTable::clearCachedJitForFunction`
   contract.
4. **Lazy refresh** in `jit_call_function_ic`'s warm path mirrors the
   method-side pattern at `JitHelpers_Objects.cpp:839-860`: when a warm
   hit finds `cachedJitFnPtr == nullptr` AND the callee crosses the
   size gate, re-probe `JitCodeCache::lookup` and write back. The size-
   gate guard on the probe keeps tiny callees off the hashmap, which is
   what blew up MYT-321 originally. Without this, callees that tier up
   to JIT after their caller's first cold-fill at the IP never get
   picked up — the common case in single-loop benchmarks.

### Summary (jit=on)

```
  Script                             min(ms)    median(ms)    instructions     calls
  arithmetic_tight_loop.mt            103.08        103.45           20017         0
  method_dispatch.mt                  105.21        106.18           14042       506
  object_alloc.mt                     623.83        637.55           12511         0
  object_alloc_nested.mt             1496.65       1505.98           16811       500
  gc_cycle_churn.mt                   240.02        242.24           26511    160900
  field_write_hot.mt                   77.50         78.03            8018         1
  field_read_hot.mt                    78.51         78.54            9020         1
  polymorphic_field_hot.mt            531.68        532.78        42000094         8
  string_ops.mt                       108.19        109.57           19019         0
  recursive.mt                        681.26        682.45           17260   2545487
  bitwise_tight_loop.mt                75.96         76.62           23019         0
  short_circuit_chain.mt               63.70         63.97           24909         0
  primitive_method_dispatch.mt        228.06        229.59           32041         0
  array_multi_alloc.mt                 35.34         35.55            9911       500
  array_multi_get.mt                  279.65        279.68           60117       500
  for_each_loop.mt                    339.08        343.08           75655      5603
  inline_monomorphic.mt                79.10         79.61           13016       501
  inline_branching.mt                  81.55         83.46           15016       501
  inline_polymorphic.mt                99.91        101.57           14051       508
  inline_polymorphic_mixed.mt         356.92        359.56           32926       508
  megamorphic_dispatch.mt             793.51        820.96           14069       512
  inline_value_object_hot.mt          168.81        169.71           12517       500
  function_call_hot.mt                 95.01         95.39           15011       500
  function_entry_tier_hot.mt           95.39         95.54           11811      1500
  async_await_tight_loop.mt           652.31        655.14           12425       501
  async_await_chain.mt               1291.86       1293.02           23325      2001
  lambda_call_hot.mt                  688.12        693.10           12521   1999501
  lambda_closure_hot.mt               710.95        717.98           12526   1999502
  generic_dispatch_hot.mt             647.02        659.71           20074      1012
  try_catch_finally_hot.mt            358.90        361.02           50019      2000
  switch_dispatch_hot.mt              488.75        489.24           14634       500
  overload_dispatch_hot.mt            494.59        494.94           34026      2001
  abstract_dispatch_hot.mt            104.00        105.51           14042       506
  cast_hot.mt                         268.12        270.22           19560       505
  collections_hash_hot.mt             638.20        639.34           32763       502
  collections_hash_user_class_hot.mt        717.83        718.19           35775       502
  collections_hashset_hot.mt          180.55        185.14           18655         1
  stream_pipeline_hot.mt              377.99        384.97         2090493    292256
  reflection_lookup_hot.mt           1465.23       1482.36           81551   1203003
  pattern_match_hot.mt                473.00        476.31           12861       500
  string_interpolation_hot.mt         215.32        217.18         7400025         0
  boxed_primitive_dispatch_hot.mt        618.11        619.73           32805         0
  boxed_bool_dispatch_hot.mt          507.32        507.71           29276         0
  boxed_string_dispatch_hot.mt        383.06        383.27           24264         0
  static_call_hot.mt                  167.35        168.81           32516      2000
  linked_list_nested_hot.mt           148.83        150.74          124921      1969
  method_chain_hot.mt                  20.99         21.00           36526      2389
  string_build_call_hot.mt             95.04         96.19           21015       500
```

### Deltas

AC benchmarks (the canary set the original revert tripped on) all stay
within ±3% of the MYT-321 baseline:

- **`generic_dispatch_hot.mt`: 639.86 → 647.02 ms (+1.1%)** — flat, no
  re-regression. The original MYT-321 attempt was +56% here.
- **`static_call_hot.mt`: 165.13 → 167.35 ms (+1.3%)** — flat.
- **`function_call_hot.mt`: 94.49 → 95.01 ms (+0.6%)** — flat.
- **`function_entry_tier_hot.mt`: 93.60 → 95.39 ms (+1.9%)** — flat.

The four AC benchmarks all have callees below
`MIN_DIRECT_CALL_INSTRUCTION_COUNT` (1–4 instructions each), so by design
they fall through to `callFunctionFromJitDirect`'s mini-interpret path
and never engage the new direct-dispatch helper. The size-gated lazy
refresh also skips them, so the hashmap-probe-per-call cost that caused
MYT-321's revert is absent on the canary set. Flat result is the
intended outcome.

### Observation: no benchmark in the suite captures a direct-dispatch win

Even with the lazy refresh landed, the post-lazy-refresh run shows no
measurable improvement on any benchmark, because the suite has no
workload with the right shape:

- Method-dispatch workloads already won under MYT-321
  (`linked_list_nested_hot.mt` is steady at ~150 ms, down from pre-MYT-321
  305 ms).
- Free-function workloads all have callees ≤ 4 instructions (the four
  AC benchmarks were designed as "tiny callee" canaries for the original
  regression). None cross the 15-instruction size gate.
- Async, lambda, collections, reflection workloads don't go through
  `CALL_STATIC` at all, so the helper never fires.

The lazy refresh is verified-correct against the
`jitDirectFunctionDispatch_pass.mt` test fixture (which constructs a
~30-instruction callee specifically to engage the direct path). Wins on
production workloads are expected wherever a `CALL_STATIC` site targets
a JIT-compiled non-trivial free-function body — those just aren't
represented in the current bench suite.

Run-to-run noise on this set is roughly ±3%; nothing in the diff
between the pre-lazy-refresh and post-lazy-refresh runs falls outside
that band, including method-side benchmarks (so the small
`method_dispatch.mt` and `megamorphic_dispatch.mt` movements are noise,
not regressions caused by the function-side IC slot work).

### Still open

- Add a `function_call_medium_hot.mt` benchmark with a 20-30 instruction
  free-function callee in a 1M-iteration loop, plus a pre-warmup pass
  that gets the callee JIT-compiled before the timed region. That's the
  missing canary for "direct dispatch actually fires," and once it
  exists the size-gate threshold becomes empirically tunable instead of
  guessed.
