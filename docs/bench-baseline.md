# mType Interpreter Baseline

Committed min wall-clock numbers from `mType.exe --benchmark`.
See `docs/benchmarks.md` for how to update.

Only the most recent run is kept here; prior runs live in git history
(`git log -p docs/bench-baseline.md`).

## 2026-05-15 — MYT-317 (bridge arena + SharedStackFrame pool + string SSO)

- Machine: dev machine (Windows 11 Home)
- Commit:  branch `MYT-317` (uncommitted; on top of `2b763e11`)
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)

MYT-317 lands three Value-layer allocation reductions in one PR:

1. **Bridge arena** (`mType/value/BridgeArena.hpp`). Every heap `Value`'s
   `TypedBridge<K, Held>` was an extra allocation per Value. `RefCounted`
   gained a virtual `destroy()` hook (default `delete this`); `BridgeBase`
   overrides it to push destructed raw memory blocks into a per-`BridgeKind`
   freelist. `makeBridge` placement-news into a recycled slot on hit.
   `PluginLoader::invalidateNativeCaches` resets the arena alongside
   `clearNativeCacheSlots`.
2. **SharedStackFrame pool** (`mType/vm/runtime/context/SharedStackFramePool.hpp`).
   Every lambda invocation paid one `std::make_shared<SharedStackFrame>()`.
   Pool mirrors `ObjectInstancePool::SlotDeleter`: cleared `locals` /
   `nameToSlot` keep their bucket arrays for the next reuse, with a
   `LOCALS_KEEP_THRESHOLD=256` shrink guard for deep nesting blowups. All
   5 `make_shared<SharedStackFrame>` call sites switched to
   `makePooledFrame()`.
3. **String SSO** (`ValueType::STRING_INLINE`). Strings ≤14 bytes live
   inline in the 16-byte `Value` (anonymous-union overlay; tag at offset 0,
   length at offset 1, 14-byte char buffer at offsets 2–15). `Value` copy /
   move switched to full-storage memcpy so the formerly-padding bytes are
   carried across. `handleInvokeStringConcat` builds inline directly when
   the combined length fits; longer results keep going through
   `StringPool::intern()`. Cross-kind `STRING ↔ STRING_INLINE` equality
   handled by a new private `stringEquals()` member; ~25 critical readers
   (printers, JSON serial, NativeBinder, JIT helpers, FFI bridges,
   reflection helpers, debugger formatters) migrated to `isAnyString` +
   `asStringView`.

`Value(InlineStringTag, const char*, uint8_t)` requires a mandatory tag
struct to dodge the `[[feedback_value_bool_pointer_decay]]` regression
that bit MYT-208.

### Summary (jit=on)

```
  Script                             min(ms)    median(ms)    instructions     calls
  arithmetic_tight_loop.mt            106.04        106.05           20017         0
  method_dispatch.mt                   98.26         98.34           14042       506
  object_alloc.mt                     614.97        622.53           12511         0
  object_alloc_nested.mt             1520.44       1520.70           16811       500
  gc_cycle_churn.mt                   224.68        225.34           26511    160900
  field_write_hot.mt                   78.42         78.55            8018         1
  field_read_hot.mt                    78.69         79.22            9020         1
  polymorphic_field_hot.mt            537.03        538.45        42000094         8
  string_ops.mt                       108.91        109.71           19019         0
  recursive.mt                        676.43        687.14           17260   2545487
  bitwise_tight_loop.mt                77.32         78.88           23019         0
  short_circuit_chain.mt               64.34         65.22           24909         0
  primitive_method_dispatch.mt        226.43        227.59           32041         0
  array_multi_alloc.mt                 36.02         36.57            9911       500
  array_multi_get.mt                  280.49        281.44           60117       500
  for_each_loop.mt                    350.81        353.18           75655      5604
  inline_monomorphic.mt                79.44         79.52           13016       501
  inline_branching.mt                  83.14         84.00           15016       501
  inline_polymorphic.mt               102.60        103.44           14051       508
  inline_polymorphic_mixed.mt         329.40        333.63           32926       508
  megamorphic_dispatch.mt             763.42        770.97           14069       512
  inline_value_object_hot.mt          165.08        165.15           12517       500
  function_call_hot.mt                 93.16         93.85           15011       500
  function_entry_tier_hot.mt           93.60         94.53           11811      1500
  async_await_tight_loop.mt           664.30        667.85           12425       501
  async_await_chain.mt               1307.43       1310.06           23325      2001
  lambda_call_hot.mt                  687.25        696.66           12521   1999501
  lambda_closure_hot.mt               712.82        720.63           12526   1999502
  generic_dispatch_hot.mt             645.72        647.95           20074      1012
  try_catch_finally_hot.mt            360.27        368.14           50019      2000
  switch_dispatch_hot.mt              497.67        499.79           14634       500
  overload_dispatch_hot.mt            495.28        499.36           34026      2001
  abstract_dispatch_hot.mt            101.20        101.27           14042       506
  cast_hot.mt                         269.01        270.26           19560       505
  collections_hash_hot.mt             628.70        633.81           32763       502
  collections_hash_user_class_hot.mt        708.83        710.92           35775       502
  collections_hashset_hot.mt          171.08        172.22           18655         1
  stream_pipeline_hot.mt              407.33        407.98         2090493    306881
  reflection_lookup_hot.mt           1948.10       1961.94           81551   1203003
  pattern_match_hot.mt                473.76        474.46           12861       500
  string_interpolation_hot.mt         217.31        217.57         7400025         0
  boxed_primitive_dispatch_hot.mt        606.92        613.26           32805         0
  boxed_bool_dispatch_hot.mt          496.16        500.10           29276         0
  boxed_string_dispatch_hot.mt        374.86        380.15           24264         0
  static_call_hot.mt                  167.43        168.06           32516      2000
  linked_list_nested_hot.mt           305.04        307.73          124921     81001
  method_chain_hot.mt                  21.16         21.42           36526      2389
  string_build_call_hot.mt             95.03         95.23           21015       500
```

### Notes on the deltas vs the prior MYT-315 baseline (in git history)

The three sub-items hit different workload shapes. Headlines:

- **`function_entry_tier_hot.mt`: 471.59 → 93.60 ms (−80.2%)** — the
  frame pool dominates this one. Every tier-up handshake pulled a fresh
  `SharedStackFrame` per invocation; the pool reuses one slot for the hot
  caller forever.
- **`function_call_hot.mt`: 185.40 → 93.16 ms (−49.7%)** — same shape.
- **`array_multi_alloc.mt`: 69.42 → 36.02 ms (−48.1%)** — bridge arena
  on the per-allocation `NATIVE_ARRAY` / `FLAT_MULTI_ARRAY` bridges.
- **`string_ops.mt`: 129.54 → 108.91 ms (−15.9%)** — string SSO. Short
  concat results skip both the bridge allocation and `StringPool::intern`.
- **`array_multi_get.mt`: 333.16 → 280.49 ms (−15.8%)** — bridge arena
  on the per-element `Value` materialization path.
- **`lambda_closure_hot.mt`: 824.90 → 712.82 ms (−13.6%)**,
  **`lambda_call_hot.mt`: 787.29 → 687.25 ms (−12.7%)** — frame pool.
- **`overload_dispatch_hot.mt`: 564.61 → 495.28 ms (−12.3%)** — boxed
  primitive dispatch shares the bridge-arena win.
- **`boxed_bool_dispatch_hot.mt`: 553.85 → 496.16 ms (−10.4%)**,
  **`boxed_string_dispatch_hot.mt`: 414.25 → 374.86 ms (−9.5%)**,
  **`boxed_primitive_dispatch_hot.mt`: 662.91 → 606.92 ms (−8.4%)** —
  same bridge-arena win on the `ValueObject` / `OBJECT_INSTANCE` paths.
- **`object_alloc.mt`: 665.32 → 614.97 ms (−7.6%)** — bridge arena.
- **`method_dispatch.mt`: 105.88 → 98.26 ms (−7.2%)** — bridge arena
  on the per-call boxed-receiver materialization.
- **`reflection_lookup_hot.mt`: 2050.19 → 1948.10 ms (−5.0%)** — bridge
  arena; reflection allocates many transient bridges.

Regressions outside the ±3% noise band:

- **`inline_value_object_hot.mt`: 144.00 → 165.08 ms (+14.6%)** — needs
  investigation. ValueObject hot path; suspect the virtual dispatch on
  `destroy()` shows up despite ostensibly only firing at refcount=0.
- **`polymorphic_field_hot.mt`: 471.65 → 537.03 ms (+13.9%)** — IC field
  load path; the new `isAnyString` checks in the auto-box gate may be
  adding a branch on every dispatch.
- **`recursive.mt`: 618.77 → 676.43 ms (+9.3%)** — fib/ack. Pure function
  calls don't touch the frame pool, so this is bridge-arena overhead or
  the auto-box-gate widening on the boxed return path.
- **`try_catch_finally_hot.mt`: 360.27 ms (+9.0%)**,
  **`inline_polymorphic_mixed.mt`: 329.40 ms (+7.2%)**,
  **`static_call_hot.mt`: 167.43 ms (+6.7%)**,
  **`switch_dispatch_hot.mt`: 497.67 ms (+6.0%)** — all in the +5–9%
  range, likely the same auto-box-gate widening pattern.

`gc_cycle_churn.mt` is new in this baseline (not in the MYT-315 run);
no delta available.

### Coverage caveats

- JIT-side handling of `STRING_INLINE` is out of scope (`OSRManager` and
  `TypeFeedbackCollector` classify it as `STRING` so the slot type stays
  stable; JIT-emitted `tag == STRING` guards reject inline values and
  fall through to the interpreter). String-SSO wins only materialize on
  the interpreter concat path; pure-JIT string benchmarks will not show
  the SSO delta until a JIT follow-up lands.
- Bridge arena bucket caps are 32 for hot kinds (`STD_STRING`,
  `OBJECT_INSTANCE`, `BYTECODE_LAMBDA`, `NATIVE_ARRAY`) and 8 for cold
  kinds. No telemetry-driven retuning yet.
- The frame pool is sized at `BUCKET_CAP=64`. The single freelist is
  shape-agnostic — frames with very wide `locals` past 256 slots are
  discarded rather than parked, so frame-explosion workloads see no
  benefit. No regression there because the discard path falls back to
  the original `delete` semantics.

### Sanity outputs (must match on re-run for same commit)

- `function_entry_tier_hot.mt`: `function_entry_tier_hot total=6000003000000`
- `myt314JitFunctionEntryTier_pass.mt` (integration fixture): `myt314 total=11325`
