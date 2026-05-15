# mType Interpreter Baseline

Committed min wall-clock numbers from `mType.exe --benchmark`.
See `docs/benchmarks.md` for how to update.

Only the most recent run is kept here; prior runs live in git history
(`git log -p docs/bench-baseline.md`).

## 2026-05-15 — MYT-315 (JIT→JIT direct dispatch after shape guard)

- Machine: dev machine (Windows 11 Home)
- Commit:  branch `MYT-315` (uncommitted; on top of `e9713ea3`)
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)

MYT-315 lands the JIT→JIT direct-dispatch path. `MethodICEntry` gains a
`cachedJit` slot populated at IC fill time (both interpreter and JIT IC
populators look up `JitCodeCache::lookup(qualifiedName)`). `emitCallMethodOp`
now tries `tryEmitDirectMethodCall` before falling back to the generic
`cc.invoke jit_call_method_ic` slow path: when a MONO/POLY IC entry has a
non-null `cachedJit`, the JIT emits an inline shape guard followed by a
`cc.invoke jit_call_method_direct` that calls the cached `JitFunction*`
through a fresh nested `JitContext`. The mini-interpreter hop
(`callMethodFromJitDirect`) is skipped on the hot path; the original IC
slow path remains as the fallback for guard miss or absent `cachedJit`.

The MYT-184 historical revert (MYT-161 `/GS`-cookie crash) is sidestepped
by routing through a C++ helper that allocates a fresh `JitContext` on its
own native frame — the callee runs through its own complete `cc.add_func`
prologue, identical to the top-level VM→JIT entry path. No shared-frame
hazard.

### Summary (jit=on)

```
  Script                             min(ms)    median(ms)    instructions     calls
  arithmetic_tight_loop.mt             98.40         99.35           20017         0
  method_dispatch.mt                  104.68        105.39           14042       506
  object_alloc.mt                     645.46        652.95           12511         0
  object_alloc_nested.mt             1496.22       1506.59           16811       500
  field_write_hot.mt                   80.91         81.26            8018         1
  field_read_hot.mt                    80.04         80.10            9020         1
  polymorphic_field_hot.mt            470.73        472.09        42000094         8
  string_ops.mt                       131.33        131.90           19019         0
  recursive.mt                        644.52        644.87           17260   2545487
  bitwise_tight_loop.mt                77.89         77.92           23019         0
  short_circuit_chain.mt               63.69         64.72           24909         0
  primitive_method_dispatch.mt        248.13        249.70           32041         0
  array_multi_alloc.mt                 69.07         69.45            9911       500
  array_multi_get.mt                  347.11        348.67           60117       500
  for_each_loop.mt                    336.09        341.22           75655      5604
  inline_monomorphic.mt                82.47         83.08           13016       501
  inline_branching.mt                  85.40         85.82           15016       501
  inline_polymorphic.mt               107.30        108.58           14051       508
  inline_polymorphic_mixed.mt        1056.54       1056.93           32926       508
  megamorphic_dispatch.mt             797.86        802.57           14069       512
  inline_value_object_hot.mt          143.92        144.03           12517       500
  function_call_hot.mt                185.47        186.80           15011       500
  function_entry_tier_hot.mt          468.45        470.91           11811      1500
  async_await_tight_loop.mt           666.55        667.00           12425       501
  async_await_chain.mt               1350.04       1352.41           23325      2001
  lambda_call_hot.mt                  795.13        796.39           12521   1999501
  lambda_closure_hot.mt               817.96        818.65           12526   1999502
  generic_dispatch_hot.mt             631.34        634.75           20074      1012
  try_catch_finally_hot.mt            332.55        332.81           50019      2000
  switch_dispatch_hot.mt              468.52        468.53           14634       500
  overload_dispatch_hot.mt            567.79        568.97           34026      2001
  abstract_dispatch_hot.mt            106.12        106.39           14042       506
  cast_hot.mt                         270.30        271.74           19560       505
  collections_hash_hot.mt             650.17        650.17           32763       502
  collections_hash_user_class_hot.mt        700.05        703.87           35775       502
  collections_hashset_hot.mt          184.56        185.64           18655         1
  stream_pipeline_hot.mt              403.68        412.14         2090493    306881
  reflection_lookup_hot.mt           2079.15       2094.45           81551   1203003
  pattern_match_hot.mt                456.55        461.04           12861       500
  string_interpolation_hot.mt         221.16        223.38         7400025         0
  boxed_primitive_dispatch_hot.mt        688.30        715.11           32805         0
  boxed_bool_dispatch_hot.mt          570.18        576.36           29276         0
  boxed_string_dispatch_hot.mt        416.25        419.12           24264         0
  static_call_hot.mt                  156.53        158.90           32516      2000
  linked_list_nested_hot.mt           315.53        322.12          124921     81001
  method_chain_hot.mt                  21.56         22.11           36526      2389
  string_build_call_hot.mt             99.36         99.58           21015       500
```

### Notes on the deltas vs the prior run (2026-05-15 MYT-314, in git history)

The MYT-315 direct path activates only when the callee was already
JIT-compiled at the time the IC entry filled — `cachedJit` is captured
once at populate-time and isn't re-checked on subsequent dispatches.
Hot benchmarks where the callee tiers up *before* the IC fills see the
benefit; benchmarks where the IC fills early (during warmup) and the
callee tiers up later still route through `jit_call_method_ic`. This is
the documented v1 behavior; a follow-up could opportunistically refresh
`cachedJit` on each IC hit.

- **`polymorphic_field_hot.mt`: 502.54 → 470.73 ms (−6.3%)** — the
  POLY direct-dispatch chain at this site picks up the JIT'd callees.
- **`inline_polymorphic_mixed.mt`: 1101.75 → 1056.54 ms (−4.1%)** —
  Big's non-inlined shape now direct-dispatches instead of routing
  through the IC mini-interpreter.
- **`megamorphic_dispatch.mt`: 824.73 → 797.86 ms (−3.3%)** — small win
  on the MEGA→POLY-eligible shapes; the dispatcher itself unchanged.
- **`object_alloc_nested.mt`: 1517.58 → 1496.22 ms (−1.4%)** — modest.
- **`function_entry_tier_hot.mt`: 475.51 → 468.45 ms (−1.5%)** —
  three-leaf hot path; callees were already JIT-compiled in time.
- **`recursive.mt`: 656.52 → 644.52 ms (−1.8%)** — fib/ack get direct
  dispatch on the recursive call sites.
- **`method_chain_hot.mt`: 21.36 → 21.56 ms (essentially flat)** —
  the dedicated MYT-315 benchmark. Suspect the receiver methods are
  not yet JIT-compiled when the IC fills (chain runs once before
  tier-up threshold); follow-up would close this.
- **`boxed_primitive_dispatch_hot.mt`: 660.84 → 688.30 ms (+4.2%)** —
  modest regression. Likely the +8B `cachedJit` slot increased
  MethodICEntry from ~90B to ~98B, marginally hurting cache locality
  on a benchmark dominated by IC traversal. Worth measuring across a
  re-run to confirm signal vs noise.
- **`boxed_bool_dispatch_hot.mt`: 537.10 → 570.18 ms (+6.2%)** — same
  bucket as the primitive variant; same suspected cause.
- **`static_call_hot.mt`: 144.27 → 156.53 ms (+8.5%)** — surprising;
  CALL_STATIC doesn't go through `emitCallMethodOp` so MYT-315
  shouldn't touch it. Possibly run-to-run noise or a layout shift in
  the JIT emitter. Re-measure before triaging.
- Everything else is within ±3%, i.e. noise.

### Sanity outputs (must match on re-run for same commit)

- `function_entry_tier_hot.mt`: `function_entry_tier_hot total=6000003000000`
- `myt314JitFunctionEntryTier_pass.mt` (integration fixture): `myt314 total=11325`
