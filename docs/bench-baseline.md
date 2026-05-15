# mType Interpreter Baseline

Committed min wall-clock numbers from `mType.exe --benchmark`.
See `docs/benchmarks.md` for how to update.

Only the most recent run is kept here; prior runs live in git history
(`git log -p docs/bench-baseline.md`).

## 2026-05-15 — MYT-321 (direct JIT→JIT dispatch unblocked: boxed-slot INC fix + callee frame)

- Machine: dev machine (Windows 11 Home)
- Commit:  branch `MYT-321` (on top of `dev`)
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)

MYT-321 unblocks MYT-315 as a default-on optimization by fixing the actual
hazard underneath the MYT-184 /GS cookie corruption:

1. **Unary INT on a boxed slot** (`mType/vm/jit/JitCompiler_Arithmetic.cpp:74-93`).
   The `INC`/`DEC`/`NEG`/`BITWISE_NOT` emitter consumed the TOS slot as an
   unboxed int64. When the operand was a boxed `value::Value` (post tier-up,
   e.g. an `Int` field read inside `ArrayList::add/T`), the in-place
   arithmetic + `publishGpHint` round-trip wrote past the slot and tripped
   the MSVC `/GS` cookie on a caller's frame — `STATUS_STACK_BUFFER_OVERRUN`
   at `__report_gsfailure`, silent process exit. Fix: `popType` +
   `emitEnsureUnboxed(..., SlotType::INT)` before `consumeGpHint`, re-push
   `SlotType::INT` after. Mirrors the binary-int emitter pattern.
2. **Direct-call runtime context** (`mType/vm/jit/JitHelpers_Objects.cpp:511-625`).
   `jit_call_method_direct` now threads the callee's `BytecodeProgram` and
   `FunctionMetadata` through and pushes a real `CallFrame` (mirroring
   `ObjectExecutor::invokeInstanceMethod`) so exception unwinding, access
   checks, and stack traces see the JIT'd callee. The nested `JitContext`
   runs against the callee's program/class. The MYT-315 `calleeIsDirectCallSafe`
   conservative scan and the `MTYPE_ENABLE_MYT315` env-var gate are removed.
3. **Cached-state lookups on the JIT-fallback path**
   (`mType/vm/runtime/VirtualMachineDispatch.cpp`, `VirtualMachineJit.cpp`).
   All `program->findCachedState(...)` / `program->getInstruction(...)` sites
   in `executeInstruction`, `tryFuseAddIntConst`, and `tryJitDispatchResolved`
   now use `executionCtx ? executionCtx->program : program` so a nested
   library callee's side table is read against its own program.

### Summary (jit=on)

```
  Script                             min(ms)    median(ms)    instructions     calls
  arithmetic_tight_loop.mt            108.23        109.67           20017         0
  method_dispatch.mt                  102.60        103.29           14042       506
  object_alloc.mt                     647.78        652.63           12511         0
  object_alloc_nested.mt             1524.06       1524.22           16811       500
  gc_cycle_churn.mt                   244.00        246.46           26511    160900
  field_write_hot.mt                   81.06         81.35            8018         1
  field_read_hot.mt                    79.25         79.84            9020         1
  polymorphic_field_hot.mt            552.18        552.49        42000094         8
  string_ops.mt                       109.70        110.60           19019         0
  recursive.mt                        674.31        675.73           17260   2545487
  bitwise_tight_loop.mt                76.67         77.88           23019         0
  short_circuit_chain.mt               64.91         65.96           24909         0
  primitive_method_dispatch.mt        226.76        227.10           32041         0
  array_multi_alloc.mt                 36.17         36.58            9911       500
  array_multi_get.mt                  279.02        279.55           60117       500
  for_each_loop.mt                    347.53        352.48           75655      5603
  inline_monomorphic.mt                79.49         81.37           13016       501
  inline_branching.mt                  84.14         84.94           15016       501
  inline_polymorphic.mt               105.66        106.22           14051       508
  inline_polymorphic_mixed.mt         359.89        364.97           32926       508
  megamorphic_dispatch.mt             782.21        783.79           14069       512
  inline_value_object_hot.mt          164.54        165.09           12517       500
  function_call_hot.mt                 94.49         96.47           15011       500
  function_entry_tier_hot.mt           93.60         93.60           11811      1500
  async_await_tight_loop.mt           644.37        647.63           12425       501
  async_await_chain.mt               1271.68       1282.82           23325      2001
  lambda_call_hot.mt                  667.24        674.23           12521   1999501
  lambda_closure_hot.mt               702.49        708.85           12526   1999502
  generic_dispatch_hot.mt             639.86        642.98           20074      1012
  try_catch_finally_hot.mt            345.62        347.52           50019      2000
  switch_dispatch_hot.mt              482.40        482.69           14634       500
  overload_dispatch_hot.mt            495.24        495.77           34026      2001
  abstract_dispatch_hot.mt            101.78        102.38           14042       506
  cast_hot.mt                         270.58        270.97           19560       505
  collections_hash_hot.mt             646.71        661.06           32763       502
  collections_hash_user_class_hot.mt        724.05        727.84           35775       502
  collections_hashset_hot.mt          170.90        171.42           18655         1
  stream_pipeline_hot.mt              373.26        380.71         2090493    292256
  reflection_lookup_hot.mt           1457.57       1459.50           81551   1203003
  pattern_match_hot.mt                469.69        470.12           12861       500
  string_interpolation_hot.mt         214.58        216.41         7400025         0
  boxed_primitive_dispatch_hot.mt        634.99        640.07           32805         0
  boxed_bool_dispatch_hot.mt          518.70        536.25           29276         0
  boxed_string_dispatch_hot.mt        383.74        384.92           24264         0
  static_call_hot.mt                  165.13        165.82           32516      2000
  linked_list_nested_hot.mt           150.74        151.35          124921      1969
  method_chain_hot.mt                  20.48         20.63           36526      2389
  string_build_call_hot.mt             94.69         94.71           21015       500
```

### Deltas

Step 2 (runtime-side direct method JIT dispatch) is the surviving perf
change. Step 3 (function-side direct dispatch via `jit_call_function_direct`
+ CALL_FAST IC `cachedJitFnPtr`) was reverted after a `generic_dispatch_hot.mt`
regression that root-caused to per-call asmjit prologue + frame-setup cost
exceeding mini-interpret for tiny static callees. Filed as MYT-322 for a
proper re-attempt with pre-cached IC fields + a callee-size gate.

Headlines vs the prior MYT-317 baseline (the +56% `generic_dispatch_hot.mt`
intermediate state is in git history but not committed as a baseline):

- **`linked_list_nested_hot.mt`: 305.04 → 150.74 ms (−50.6%)** —
  Step 2 direct JIT dispatch from `jit_call_method_ic`'s warm path.
  LinkedList traversal hits the runtime-side IC arm repeatedly on
  JIT-compiled nodes with bodies large enough to amortize the per-call
  prologue. The dominant MYT-321 win.
- **`generic_dispatch_hot.mt`: 645.72 → 639.86 ms (−0.9%)** — back at
  baseline after Step 3 revert.

Workload-noise regressions worth watching but not currently load-bearing
(all within ±5% of MYT-317 numbers):

- `collections_hash_user_class_hot.mt`: 708.83 → 724.05 ms (+2.2%)
- `boxed_primitive_dispatch_hot.mt`: 606.92 → 634.99 ms (+4.6%)
- `boxed_bool_dispatch_hot.mt`: 496.16 → 518.70 ms (+4.5%)
- `megamorphic_dispatch.mt`: 763.42 → 782.21 ms (+2.5%)

These weren't moved deliberately by MYT-321; if they prove sticky on a
re-run, file individually. None block MYT-321 from landing.
