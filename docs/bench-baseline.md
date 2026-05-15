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
  arithmetic_tight_loop.mt            107.08        108.57           20017         0
  method_dispatch.mt                  103.66        103.81           14042       506
  object_alloc.mt                     626.50        633.54           12511         0
  object_alloc_nested.mt             1508.56       1514.07           16811       500
  gc_cycle_churn.mt                   244.28        244.33           26511    160900
  field_write_hot.mt                   79.38         81.17            8018         1
  field_read_hot.mt                    80.16         81.09            9020         1
  polymorphic_field_hot.mt            541.79        547.52        42000094         8
  string_ops.mt                       108.79        110.71           19019         0
  recursive.mt                        677.03        678.33           17260   2545487
  bitwise_tight_loop.mt                79.54         81.62           23019         0
  short_circuit_chain.mt               64.23         64.39           24909         0
  primitive_method_dispatch.mt        219.01        220.47           32041         0
  array_multi_alloc.mt                 35.35         35.77            9911       500
  array_multi_get.mt                  274.91        274.91           60117       500
  for_each_loop.mt                    340.19        341.60           75655      5603
  inline_monomorphic.mt                78.67         79.38           13016       501
  inline_branching.mt                  81.63         82.33           15016       501
  inline_polymorphic.mt               103.75        104.23           14051       508
  inline_polymorphic_mixed.mt         355.64        356.41           32926       508
  megamorphic_dispatch.mt             761.49        766.13           14069       512
  inline_value_object_hot.mt          162.54        162.63           12517       500
  function_call_hot.mt                 92.15         92.19           15011       500
  function_entry_tier_hot.mt           92.29         93.11           11811      1500
  async_await_tight_loop.mt           634.83        652.43           12425       501
  async_await_chain.mt               1263.85       1269.49           23325      2001
  lambda_call_hot.mt                  670.48        674.28           12521   1999501
  lambda_closure_hot.mt               692.11        702.98           12526   1999502
  generic_dispatch_hot.mt             642.51        644.69           20074      1012
  try_catch_finally_hot.mt            351.94        354.55           50019      2000
  switch_dispatch_hot.mt              481.10        481.77           14634       500
  overload_dispatch_hot.mt            487.81        489.69           34026      2001
  abstract_dispatch_hot.mt            104.00        104.15           14042       506
  cast_hot.mt                         272.09        274.68           19560       505
  collections_hash_hot.mt             632.36        635.58           32763       502
  collections_hash_user_class_hot.mt        694.90        703.48           35775       502
  collections_hashset_hot.mt          170.35        174.25           18655         1
  stream_pipeline_hot.mt              372.53        373.57         2090493    292256
  reflection_lookup_hot.mt           1445.92       1466.48           81551   1203003
  pattern_match_hot.mt                475.08        477.85           12861       500
  string_interpolation_hot.mt         216.64        217.77         7400025         0
  boxed_primitive_dispatch_hot.mt        612.78        618.17           32805         0
  boxed_bool_dispatch_hot.mt          516.64        522.81           29276         0
  boxed_string_dispatch_hot.mt        385.99        392.62           24264         0
  static_call_hot.mt                  169.03        169.48           32516      2000
  linked_list_nested_hot.mt           279.76        281.96          124921      1969
  method_chain_hot.mt                  20.84         21.06           36526      2389
  string_build_call_hot.mt             94.37         94.78           21015       500
```

A fresh bench-baseline replacement is expected once the Step 2 (runtime-side
direct method dispatch in `jit_call_method_ic`) and Step 3 (function-side
`jit_call_function_direct` + CALL_FAST `cachedJit`) follow-up sweeps land —
the wins should compound on `static_call_hot.mt`, `generic_dispatch_hot.mt`,
`function_call_hot.mt`, and `function_entry_tier_hot.mt`.
