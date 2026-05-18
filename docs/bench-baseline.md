# mType Interpreter Baseline

Committed min wall-clock numbers from `mType.exe --benchmark`.
See `docs/benchmarks.md` for how to update.

Only the most recent run is kept here; prior runs live in git history
(`git log -p docs/bench-baseline.md`).

## 2026-05-18 - ExecutionContext + BytecodeOptimizationService deepening

- Machine: dev machine (Windows 11 Home)
- Commit: branch with ExecutionContext deepening (vm/environment/loadedPrograms
  removed from `ExecutionContext`, threaded through vm-coupled executor
  constructors) and BytecodeOptimizationService extraction (post-AST passes
  moved out of `BytecodeCompiler::compile` into a pass pipeline mirroring the
  AST-side `OptimizationService`)
- Build: Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)

The same five post-AST bytecode passes run in the same registration order
(LoopOptimization → Peephole → TrivialSetterInlining → TrivialGetterInlining →
LocalArrayFusion), so emitted bytecode is intended to be byte-identical to the
previous baseline. Instruction/call counts confirm the shape across all
benchmarks; timing movement is within run-to-run variance.

### Summary (jit=on)

```
  Script                             min(ms)    median(ms)    instructions     calls
  arithmetic_tight_loop.mt            108.57        111.36           20017         0
  method_dispatch.mt                  103.25        103.46           14042       506
  object_alloc.mt                     625.26        627.17           12511         0
  object_alloc_nested.mt             1546.85       1550.49           16812       500
  gc_cycle_churn.mt                   238.09        238.54           26511    160900
  field_write_hot.mt                   77.82         79.07            8018         1
  field_read_hot.mt                    76.38         77.12            9020         1
  polymorphic_field_hot.mt            496.95        498.91        42000094         8
  string_ops.mt                       110.56        110.87           19019         0
  recursive.mt                        668.84        669.44           17260   2545487
  bitwise_tight_loop.mt                79.52         79.80           23019         0
  short_circuit_chain.mt               64.84         64.87           24909         0
  primitive_method_dispatch.mt        230.66        231.55           32038         0
  array_multi_alloc.mt                 35.82         36.29            9911       500
  array_multi_get.mt                  276.88        279.54           60117       500
  for_each_loop.mt                    335.10        336.94           75653      5603
  inline_monomorphic.mt                81.19         82.67           13016       501
  inline_branching.mt                  82.63         82.72           15016       501
  inline_polymorphic.mt               104.79        105.89           14051       508
  inline_polymorphic_mixed.mt         365.83        366.39           32926       508
  megamorphic_dispatch.mt             772.21        782.91           14069       512
  inline_value_object_hot.mt          163.56        163.80           12517       500
  function_call_hot.mt                 92.51         93.40           15011       500
  function_entry_tier_hot.mt           93.59         93.63           11811      1500
  function_call_medium_hot.mt         177.57        177.81           13911       500
  async_await_tight_loop.mt           666.95        668.48           12422       501
  async_await_chain.mt               1288.10       1289.50           23322      2001
  lambda_call_hot.mt                  681.80        689.57           12521   1999501
  lambda_closure_hot.mt               713.01        715.81           12526   1999502
  generic_dispatch_hot.mt             643.60        652.46           20074      1012
  try_catch_finally_hot.mt            348.02        351.27           50019      2000
  switch_dispatch_hot.mt              482.56        485.71           14634       500
  overload_dispatch_hot.mt            508.18        512.98           34026      2001
  abstract_dispatch_hot.mt            108.92        109.01           14042       506
  cast_hot.mt                         277.22        281.24           19560       505
  collections_hash_hot.mt             616.21        616.44           32761       502
  collections_hash_user_class_hot.mt        684.22        685.21           35773       502
  collections_hashset_hot.mt          168.00        169.30           18653         1
  stream_pipeline_hot.mt              368.33        369.36         2090491    292256
  reflection_lookup_hot.mt           1465.41       1466.87           81549   1203003
  pattern_match_hot.mt                461.88        462.39           12861       500
  string_interpolation_hot.mt         216.04        216.18         7400025         0
  boxed_primitive_dispatch_hot.mt        630.14        630.50           32802         0
  boxed_bool_dispatch_hot.mt          526.18        529.37           29276         0
  boxed_string_dispatch_hot.mt        392.36        392.68           24261         0
  static_call_hot.mt                  168.66        168.91           32516      2000
  linked_list_nested_hot.mt           146.12        146.66          124919      1969
  method_chain_hot.mt                  21.89         22.24           36526      2389
  string_build_call_hot.mt             96.67         97.32           21015       500
```

### Notes

- Bytecode-pass extraction is a pure plumbing change — the same code runs in
  the same order. Instruction/call counts match the prior baseline within
  expected noise (a few benchmarks show small `+1`/`-1` shifts in instruction
  counts from compiler-side changes unrelated to the pass pipeline).
- `collections_hash_hot`, `collections_hash_user_class_hot`,
  `linked_list_nested_hot`, and `stream_pipeline_hot` stay on their MYT-324
  shape — no OSR regressions introduced by the ExecutionContext threading.
- Timing movement outside instruction/call shape changes is treated as normal
  run-to-run wall-clock variance.
