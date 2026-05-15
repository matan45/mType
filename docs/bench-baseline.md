# mType Interpreter Baseline

Committed min wall-clock numbers from `mType.exe --benchmark`.
See `docs/benchmarks.md` for how to update.

Only the most recent run is kept here; prior runs live in git history
(`git log -p docs/bench-baseline.md`).

## 2026-05-15 — MYT-315 (JIT→JIT direct dispatch with safety filter)

- Machine: dev machine (Windows 11 Home)
- Commit:  branch `MYT-315` (uncommitted; on top of `e9713ea3`)
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)

MYT-315 emits a shape-guarded direct `cc.invoke jit_call_method_direct`
in place of `cc.invoke jit_call_method_ic` whenever the IC entry's callee
is JIT-compiled. The cachedJit pointer is found by a fresh lookup against
`s.codeCache` at emit time (the IC-fill-time slot is typically null because
the callee tiers up *after* the IC fills). The direct path skips
`callMethodFromJitDirect`'s mini-interpreter hop.

The MYT-184 `/GS` cookie hazard turns out to still apply to non-inlining
JIT→JIT call boundaries — `errorLargeExceptionData_pass.mt` reproduced
the silent `STATUS_STACK_BUFFER_OVERRUN` crash when an OSR'd outer caller
direct-dispatched `ArrayList::add` (a callee containing a nested
`this.resize()` call). Until [MYT-321](https://matan33214.atlassian.net/browse/MYT-321)
roots out the actual frame-layout invariant being violated, the
`calleeIsDirectCallSafe` filter in `JitCompiler_Objects.cpp` rejects any
callee whose bytecode contains a CALL-family / NEW_OBJECT / AWAIT / THROW
opcode. This keeps us to single-level nesting where the empirical
evidence shows the hazard doesn't fire. Value-class receivers are also
excluded because the existing mini-interpreter path materializes them
via `loadFromValueObject` before dispatching, which the direct path skips.

### Summary (jit=on)

```
  Script                             min(ms)    median(ms)    instructions     calls
  arithmetic_tight_loop.mt            107.72        111.47           20017         0
  method_dispatch.mt                  105.88        108.01           14042       506
  object_alloc.mt                     665.32        666.12           12511         0
  object_alloc_nested.mt             1485.85       1499.53           16811       500
  field_write_hot.mt                   77.59         78.41            8018         1
  field_read_hot.mt                    79.02         79.64            9020         1
  polymorphic_field_hot.mt            471.65        473.54        42000094         8
  string_ops.mt                       129.54        130.14           19019         0
  recursive.mt                        618.77        622.04           17260   2545487
  bitwise_tight_loop.mt                75.18         75.38           23019         0
  short_circuit_chain.mt               61.40         62.18           24909         0
  primitive_method_dispatch.mt        236.00        239.82           32041         0
  array_multi_alloc.mt                 69.42         69.56            9911       500
  array_multi_get.mt                  333.16        333.50           60117       500
  for_each_loop.mt                    338.86        342.77           75655      5604
  inline_monomorphic.mt                81.20         81.38           13016       501
  inline_branching.mt                  84.48         84.77           15016       501
  inline_polymorphic.mt               105.43        105.75           14051       508
  inline_polymorphic_mixed.mt         307.33        308.05           32926       508
  megamorphic_dispatch.mt             792.47        796.11           14069       512
  inline_value_object_hot.mt          144.00        144.01           12517       500
  function_call_hot.mt                185.40        189.57           15011       500
  function_entry_tier_hot.mt          471.59        472.25           11811      1500
  async_await_tight_loop.mt           656.27        663.09           12425       501
  async_await_chain.mt               1291.19       1294.86           23325      2001
  lambda_call_hot.mt                  787.29        790.93           12521   1999501
  lambda_closure_hot.mt               824.90        825.49           12526   1999502
  generic_dispatch_hot.mt             625.66        628.17           20074      1012
  try_catch_finally_hot.mt            330.36        331.63           50019      2000
  switch_dispatch_hot.mt              469.41        472.51           14634       500
  overload_dispatch_hot.mt            564.61        567.77           34026      2001
  abstract_dispatch_hot.mt            105.99        106.04           14042       506
  cast_hot.mt                         270.91        271.51           19560       505
  collections_hash_hot.mt             648.90        650.13           32763       502
  collections_hash_user_class_hot.mt        712.31        715.23           35775       502
  collections_hashset_hot.mt          177.89        179.14           18655         1
  stream_pipeline_hot.mt              404.53        405.06         2090493    306881
  reflection_lookup_hot.mt           2050.19       2071.50           81551   1203003
  pattern_match_hot.mt                463.26        464.06           12861       500
  string_interpolation_hot.mt         221.64        222.68         7400025         0
  boxed_primitive_dispatch_hot.mt        662.91        664.89           32805         0
  boxed_bool_dispatch_hot.mt          553.85        554.06           29276         0
  boxed_string_dispatch_hot.mt        414.25        416.73           24264         0
  static_call_hot.mt                  156.93        158.68           32516      2000
  linked_list_nested_hot.mt           318.91        320.10          124921     81001
  method_chain_hot.mt                  20.96         21.65           36526      2389
  string_build_call_hot.mt             99.64        101.29           21015       500
```

### Notes on the deltas vs the prior MYT-314 baseline (in git history)

The headline is `inline_polymorphic_mixed.mt`: **1101.75 → 307.33 ms (−72.1%)**.
This is the largest single-benchmark improvement landed since the
bytecode VM cutover. The mechanism: Big's body fails the inliner's
`CALLEE_TOO_BIG` gate, so the inliner's POLY-rejected branch hands
control to MYT-315's direct dispatch instead of `jit_call_method_ic`'s
mini-interpreter. Big.compute is 63 bytecode ops of pure arithmetic
with no nested calls — exactly the shape the `calleeIsDirectCallSafe`
filter is designed to accept.

- **`inline_polymorphic_mixed.mt`: 1101.75 → 307.33 ms (−72.1%)** — see
  headline above.
- **`async_await_chain.mt`: 1339.00 → 1291.19 ms (−3.6%)** — modest;
  the await chain's resumption callees were inliner-rejected and now
  direct-dispatch.
- **`recursive.mt`: 656.52 → 618.77 ms (−5.7%)** — fib/ack hit the
  direct path on the recursive call sites once their bodies tier up.
- **`primitive_method_dispatch.mt`: 247.64 → 236.00 ms (−4.7%)**.
- **`array_multi_get.mt`: 348.74 → 333.16 ms (−4.5%)**.
- **`collections_hashset_hot.mt`: 181.16 → 177.89 ms (−1.8%)**.
- Everything else within ±3% of the prior MYT-314 numbers — i.e. noise.
- No regressions outside ±3% noise band. Notably the `boxed_*_dispatch`
  and `static_call_hot` regressions from the first MYT-315 attempt
  (the run with the broken `cachedJit` populate) have all recovered.

### Coverage caveats

The `calleeIsDirectCallSafe` filter excludes callees that contain
nested calls / `NEW_OBJECT` / `AWAIT` / `THROW`. That's the right
choice for safety today, but it leaves perf on the table:

- `method_chain_hot.mt` stays flat (callees are tiny enough that the
  inliner takes them — MYT-315 has no work to do here).
- ArrayList / HashMap / collection-heavy benchmarks don't move (their
  methods have nested calls and trip the filter).
- Methods that construct new objects (factory patterns, builder
  chains) are excluded.

MYT-321 captures the work needed to safely lift the filter and unlock
direct-dispatch for these workloads.

### Sanity outputs (must match on re-run for same commit)

- `function_entry_tier_hot.mt`: `function_entry_tier_hot total=6000003000000`
- `myt314JitFunctionEntryTier_pass.mt` (integration fixture): `myt314 total=11325`
