# mType Interpreter Baseline

Committed min wall-clock numbers from `mType.exe --benchmark`.
See `docs/benchmarks.md` for how to update.

Only the most recent run is kept here; prior runs live in git history
(`git log -p docs/bench-baseline.md`).

## 2026-05-18 - POLY width 4→8 + wide IC tier + MYT-346 fix

- Machine: dev machine (Windows 11 Home)
- Commit: branch `MYT-346` (on top of the MYT-346 fix that landed value-class
  write-method inlining via materialised temp). Adds the `IC_MAX_POLYMORPHIC_ENTRIES`
  bump from 4 → 8 (`InlineCacheTypes.hpp`) so the inline POLY array absorbs
  the 5-8 shape range without falling to the wide tier, plus the
  `WideMethodICTable` (16-slot open-addressed, lazily allocated on POLY→MEGA)
  consulted from both the interpreter dispatch (`InlineCacheExecutor.cpp`)
  and the JIT helper (`JitHelpers_Objects.cpp`). Companion Phase 2a inline
  AWAIT fast path (`JitCompiler_Objects.cpp::emitAwaitOp`) and Phase 2b
  lambda shared_ptr→raw-pointer fix (`VariableExecutor.hpp`).
- Build: Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)

### Headline movements vs prior MYT-346-only baseline

| Benchmark | Prior | New | Δ | Driver |
| --- | ---: | ---: | ---: | --- |
| `megamorphic_dispatch.mt` | 865.65 | 103.32 | **−88%** | 6 shapes now fit the wider POLY-8 inline array |
| `interface_dispatch_collections_hot.mt` | 1743.26 | 281.79 | **−84%** | 6-impl handler list + iterator dispatch fits POLY-8 |
| `switch_on_string_hot.mt` | 1002.07 | 672.49 | **−33%** | wider POLY absorbs the case-arm shape variation |
| `for_each_loop.mt` | 333.52 | 137.41 | **−59%** | wider POLY absorbs ArrayList iterator-internal shapes |
| `overload_dispatch_hot.mt` | 527.57 | 493.13 | −7% | wider POLY |
| `boxed_primitive_dispatch_hot.mt` | 668.38 | 628.18 | −6% | wider POLY across boxed-Int/Float/Bool/String |
| `lambda_call_hot.mt` | 723.70 | 658.33 | −9% | Phase 2b shared_ptr atomic op removal |
| `lambda_closure_hot.mt` | 729.90 | 693.78 | −5% | Phase 2b |
| `async_await_chain.mt` | 1268.53 | 1255.64 | −1% | Phase 2a inline AWAIT (cc.invoke still dominates) |
| `value_class_mut_hot.mt` | 492.32 | 485.92 | flat | unchanged by these passes (MYT-346 fix already landed) |

The IC width bump is the dominant lever: the 6-shape `megamorphic_dispatch`
fixture no longer hits MEGA at all (8-slot POLY array absorbs it with one
linear scan and direct dispatch). `interface_dispatch_collections_hot` and
`switch_on_string_hot` both benefit similarly — their underlying dispatch
sites had been falling off the 4-slot POLY cliff.

`object_alloc_nested.mt` (1492→1507ms) and `reflection_lookup_hot.mt`
(1450→1467ms) move within run-to-run variance — Phase 3b's trivial-init
fast path doesn't apply where the class is `Point` inside the
`distanceSq` helper (the constructor-assigned set is complete *for the
self class*, but the field-init pass still walks the hierarchy and
runs the unordered_map iteration; further win requires extending the
skippable predicate or fully elision-by-escape-analysis).

### Summary (jit=on)

```
  Script                             min(ms)    median(ms)    instructions     calls
  arithmetic_tight_loop.mt            106.32        106.58           20017         0
  method_dispatch.mt                  100.48        101.00           14042       506
  object_alloc.mt                     624.35        626.89           12511         0
  object_alloc_nested.mt             1507.54       1509.03           16812       500
  gc_cycle_churn.mt                   242.08        242.14           26511    160900
  field_write_hot.mt                   82.29         82.36            8018         1
  field_read_hot.mt                    83.64         83.97            9020         1
  polymorphic_field_hot.mt            506.04        506.54        42000094         8
  string_ops.mt                       107.48        108.84           19019         0
  recursive.mt                        663.91        665.26           17260   2545487
  bitwise_tight_loop.mt                77.02         77.21           23019         0
  short_circuit_chain.mt               63.38         63.51           24909         0
  primitive_method_dispatch.mt        226.18        228.33           32038         0
  array_multi_alloc.mt                 34.93         35.24            9911       500
  array_multi_get.mt                  279.73        281.14           60117       500
  for_each_loop.mt                    137.41        138.02           57181      1606
  inline_monomorphic.mt                79.10         79.70           13016       501
  inline_branching.mt                  84.19         84.28           15016       501
  inline_polymorphic.mt               105.36        105.91           14051       508
  inline_polymorphic_mixed.mt         359.69        370.68           32926       508
  megamorphic_dispatch.mt             103.32        103.68           14069       512
  inline_value_object_hot.mt          164.07        169.63           12517       500
  function_call_hot.mt                 91.94         92.12           15011       500
  function_entry_tier_hot.mt           93.32         93.56           11811      1500
  function_call_medium_hot.mt         166.27        167.83           13911       500
  async_await_tight_loop.mt           633.15        636.14           12422       501
  async_await_chain.mt               1255.64       1258.53           23322      2001
  lambda_call_hot.mt                  658.33        682.55           12521   1999501
  lambda_closure_hot.mt               693.78        696.92           12526   1999502
  generic_dispatch_hot.mt             659.17        661.90           20074      1012
  try_catch_finally_hot.mt            351.77        351.81           50019      2000
  switch_dispatch_hot.mt              468.38        468.48           14634       500
  overload_dispatch_hot.mt            493.13        493.91           34026      2001
  abstract_dispatch_hot.mt            102.34        102.87           14042       506
  cast_hot.mt                         271.23        273.55           19560       505
  collections_hash_hot.mt             616.27        618.95           32761       502
  collections_hash_user_class_hot.mt        679.93        680.69           35773       502
  collections_hashset_hot.mt          170.97        172.19           18653         1
  stream_pipeline_hot.mt              359.86        363.26         2090491    292256
  reflection_lookup_hot.mt           1467.36       1468.23           81549   1203003
  pattern_match_hot.mt                453.02        453.68           12861       500
  string_interpolation_hot.mt         213.37        213.53         7400025         0
  boxed_primitive_dispatch_hot.mt        628.18        633.75           32802         0
  boxed_bool_dispatch_hot.mt          521.65        524.20           29276         0
  boxed_string_dispatch_hot.mt        398.31        402.50           24261         0
  static_call_hot.mt                  171.07        171.12           32516      2000
  linked_list_nested_hot.mt           142.53        145.13          124919      1969
  method_chain_hot.mt                  21.31         21.39           36526      2389
  string_build_call_hot.mt             95.01         96.33           21015       500
  deep_inheritance_hot.mt              85.40         85.72           34531      3006
  interface_dispatch_collections_hot.mt        281.79        281.98           25979      1029
  cast_miss_hot.mt                    200.06        200.70           16459       506
  exception_throw_hot.mt              552.11        553.87         6450035    250001
  ic_transition_hot.mt                318.36        319.99           14243       533
  switch_on_string_hot.mt             672.49        675.93        33000062   1000000
  substring_hot.mt                     83.62         84.18           13013         0
  gc_churn_intense_hot.mt             808.92        821.77           11612       500
  array_literal_alloc_hot.mt          106.71        107.53           29510         0
  value_class_mut_hot.mt              485.92        486.36           14516       500
  int_only_arith_hot.mt                38.02         38.06           14010         0
```

### Notes

- The headline win is the IC width bump compounding with the wider tier:
  benchmarks that were sitting on the 4-slot POLY edge collapse cleanly.
  `interface_dispatch_collections_hot` (6 `Handler` impls) and the
  `for_each_loop` ArrayList iterator both also pick this up — their inner
  dispatch sites were the long-pole MEGA cases.
- `value_class_mut_hot.mt` continues to agree with the `--no-jit` reference
  post-MYT-346; the canary stays green.
- Slowest entries to keep eyes on:
  - `object_alloc_nested.mt 1507` — `Point` field-init pass; not covered by
    Phase 3b's trivial-init fast path (`distanceSq` allocates inside the
    helper, so the helper-class is fresh per call but the init walk still
    fires). Further win needs escape-analysis-driven elision.
  - `reflection_lookup_hot.mt 1467` — runtime-side caches are in place;
    remaining cost is `new Field` / `new Method` wrapper allocation in
    `Class.mt`, which is a script-side optimization.
  - `async_await_chain.mt 1255` — Phase 2a saved the helper-call cost on the
    PROMISE_INT fast path, but the `cc.invoke` frame setup still dominates.
    Full Phase 2d wide-cache inline emit + a no-cc-invoke AWAIT path would
    attack this next.
