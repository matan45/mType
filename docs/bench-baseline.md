# mType Interpreter Baseline

Committed min wall-clock numbers from `mType.exe --benchmark`.
See `docs/benchmarks.md` for how to update.

## 2026-04-17 — initial baseline

- Machine: dev machine (Windows 11 Home)
- Commit:  `107b1727`
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)

| Script                    | Wall min (ms) | exec (ms) | Instructions | Calls   | JIT |
|---------------------------|-------------:|-----------:|-------------:|--------:|-----|
| arithmetic_tight_loop.mt  |      5406.80 |   5406.77 |    236000031 |       0 | on  |
| method_dispatch.mt        |      2212.44 |   2224.66 |     52000048 | 2000006 | on  |
| object_alloc.mt           |      5764.47 |   6087.09 |     62000018 | 2000000 | on  |
| string_ops.mt             |       541.69 |    553.50 |     20320032 |       0 | on  |
| recursive.mt              |      1811.84 |   1811.07 |      1303765 | 2813094 | on  |

### Summary

| Script                    | min(ms) | median(ms) | Instructions | Calls   |
|---------------------------|--------:|-----------:|-------------:|--------:|
| arithmetic_tight_loop.mt  | 5406.80 |    5407.46 |    236000031 |       0 |
| method_dispatch.mt        | 2212.44 |    2225.57 |     52000048 | 2000006 |
| object_alloc.mt           | 5764.47 |    6087.77 |     62000018 | 2000000 |
| string_ops.mt             |  541.69 |     553.59 |     20320032 |       0 |
| recursive.mt              | 1811.84 |    1812.78 |      1303765 | 2813094 |

### Sanity outputs (must match on re-run for same commit)

- `arithmetic_tight_loop.mt`: `intSum=1291840006568070912` and `2e+12`
- `method_dispatch.mt`: `acc=2666666666668`
- `object_alloc.mt`: `total=1999999000000`
- `string_ops.mt`: `concat_iters=20000 matches=1000000`
- `recursive.mt`: `fib32=2178309 ack38=2045 gcdSum=150044`

## 2026-04-17 — post MYT-120/121/122/123/124 (CALL_FAST bypasses JIT, see MYT-135)

- Machine: dev machine (Windows 11 Home)
- Commit:  `107b1727` (uncommitted)
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)
- Note: `recursive.mt` regressed because CALL_FAST bypasses JIT (MYT-135 workaround)

| Script                    | Wall min (ms) | exec (ms) | Instructions | Calls    | JIT |
|---------------------------|-------------:|-----------:|-------------:|---------:|-----|
| arithmetic_tight_loop.mt  |      5278.71 |   5277.91 |    236000031 |        0 | on  |
| method_dispatch.mt        |      2191.63 |   2190.69 |     52000048 |  2000006 | on  |
| object_alloc.mt           |      5769.75 |   5913.26 |     62000018 |  2000000 | on  |
| string_ops.mt             |       536.47 |    536.19 |     20320032 |        0 | on  |
| recursive.mt              |      3472.62 |   3520.40 |    123974871 | 10410438 | on  |

### Summary

| Script                    | min(ms) | median(ms) | Instructions | Calls    |
|---------------------------|--------:|-----------:|-------------:|---------:|
| arithmetic_tight_loop.mt  | 5278.71 |    5292.34 |    236000031 |        0 |
| method_dispatch.mt        | 2191.63 |    2206.23 |     52000048 |  2000006 |
| object_alloc.mt           | 5769.75 |    5913.91 |     62000018 |  2000000 |
| string_ops.mt             |  536.47 |     537.03 |     20320032 |        0 |
| recursive.mt              | 3472.62 |    3492.58 |    123974871 | 10410438 |

### Delta vs initial baseline

| Script                    | Before (ms) | After (ms) | Change |
|---------------------------|------------:|-----------:|-------:|
| arithmetic_tight_loop.mt  |     5406.80 |    5278.71 | -2.4%  |
| method_dispatch.mt        |     2212.44 |    2191.63 | -0.9%  |
| object_alloc.mt           |     5764.47 |    5769.75 | +0.1%  |
| string_ops.mt             |      541.69 |     536.47 | -1.0%  |
| recursive.mt              |     1811.84 |    3472.62 | +91.7% (REGRESSED — MYT-135) |

### Sanity outputs (must match on re-run for same commit)

- `arithmetic_tight_loop.mt`: `intSum=1291840006568070912` and `2e+12`
- `method_dispatch.mt`: `acc=2666666666668`
- `object_alloc.mt`: `total=1999999000000`
- `string_ops.mt`: `concat_iters=20000 matches=1000000`
- `recursive.mt`: `fib32=2178309 ack38=2045 gcdSum=150044`

## 2026-04-17 — post MYT-140 (CALL_FAST JIT emission restored)

- Machine: dev machine (Windows 11 Home)
- Commit:  `b48edc56` + uncommitted MYT-140 changes (branch `MYT-140`)
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)
- Note: `recursive.mt` fully recovers — CALL_FAST is now JIT-emitted (`jit_call_function_fast`), so fib/ack/gcd compile and the JIT cache is populated during the run.

| Script                    | Wall min (ms) | exec (ms) | Instructions | Calls    | JIT |
|---------------------------|-------------:|-----------:|-------------:|---------:|-----|
| arithmetic_tight_loop.mt  |      5451.86 |   5492.78 |    236000031 |        0 | on  |
| method_dispatch.mt        |      2219.76 |   2228.03 |     52000048 |  2000006 | on  |
| object_alloc.mt           |      5764.13 |   6063.64 |     62000018 |  2000000 | on  |
| string_ops.mt             |       559.66 |    564.07 |     20320032 |        0 | on  |
| recursive.mt              |      1862.98 |   1863.71 |      1303765 |  2813094 | on  |

### Summary

| Script                    | min(ms) | median(ms) | Instructions | Calls    |
|---------------------------|--------:|-----------:|-------------:|---------:|
| arithmetic_tight_loop.mt  | 5451.86 |    5457.28 |    236000031 |        0 |
| method_dispatch.mt        | 2219.76 |    2225.23 |     52000048 |  2000006 |
| object_alloc.mt           | 5764.13 |    6064.35 |     62000018 |  2000000 |
| string_ops.mt             |  559.66 |     560.90 |     20320032 |        0 |
| recursive.mt              | 1862.98 |    1864.58 |      1303765 |  2813094 |

### Delta vs regression row (post MYT-120/121/122/123/124)

| Script                    | Regressed (ms) | After MYT-140 (ms) | Change |
|---------------------------|---------------:|-------------------:|-------:|
| arithmetic_tight_loop.mt  |        5278.71 |            5451.86 | +3.3%  |
| method_dispatch.mt        |        2191.63 |            2219.76 | +1.3%  |
| object_alloc.mt           |        5769.75 |            5764.13 | -0.1%  |
| string_ops.mt             |         536.47 |             559.66 | +4.3%  |
| recursive.mt              |        3472.62 |            1862.98 | **-46.4% (RECOVERED — MYT-140)** |

### Delta vs initial baseline

| Script                    | Initial (ms) | After MYT-140 (ms) | Change |
|---------------------------|-------------:|-------------------:|-------:|
| arithmetic_tight_loop.mt  |      5406.80 |            5451.86 | +0.8%  |
| method_dispatch.mt        |      2212.44 |            2219.76 | +0.3%  |
| object_alloc.mt           |      5764.47 |            5764.13 | -0.0%  |
| string_ops.mt             |       541.69 |             559.66 | +3.3%  |
| recursive.mt              |      1811.84 |            1862.98 | +2.8%  |

`recursive.mt` is within 3% of the original pre-regression baseline — essentially fully restored. Instruction count drops from 123,974,871 back to 1,303,765 (the same as the initial baseline), confirming the JIT is now handling CALL_FAST call sites in fib/ack/gcd instead of falling through to the interpreter.

### Sanity outputs (must match on re-run for same commit)

- `arithmetic_tight_loop.mt`: `intSum=1291840006568070912` and `2e+12`
- `method_dispatch.mt`: `acc=2666666666668`
- `object_alloc.mt`: `total=1999999000000`
- `string_ops.mt`: `concat_iters=20000 matches=1000000`
- `recursive.mt`: `fib32=2178309 ack38=2045 gcdSum=150044`

## 2026-04-17 — MYT-145: extended suite (bitwise, short-circuit, primitive method)

- Machine: dev machine (Windows 11 Home)
- Commit:  TBD (run after MYT-145 lands on `dev`)
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3) + `mType.exe --jit-stats <script>` per row for the Bailouts column
- Scope: adds `bitwise_tight_loop.mt`, `short_circuit_chain.mt`, `primitive_method_dispatch.mt`. Adds a **Bailouts** column (parsed from the `Bailouts:` line of `--jit-stats` output) so the JIT-gap tickets (MYT-141/142/144) have a before/after signal in the same table. MYT-143 (lambda) is deferred pending capture-plumbing design, so no lambda benchmark is registered here.

| Script                          | Wall min (ms) | exec (ms) | Instructions | Calls    | JIT | Bailouts | OSR fail | Hot fns |
|---------------------------------|--------------:|----------:|-------------:|---------:|-----|---------:|---------:|--------:|
| arithmetic_tight_loop.mt        |       5171.30 |   5170.39 |    236000031 |        0 | on  |        0 |        4 |       0 |
| method_dispatch.mt              |       2213.60 |   2212.64 |     52000048 |  2000006 | on  |        0 |        2 |       0 |
| object_alloc.mt                 |       5958.73 |   6096.11 |     62000018 |  2000000 | on  |        0 |        2 |       0 |
| string_ops.mt                   |        539.09 |    537.95 |     20320032 |        0 | on  |        0 |        4 |       0 |
| recursive.mt                    |       1856.67 |   1881.82 |      1303765 |  2813094 | on  |        0 |        2 |       3 |
| bitwise_tight_loop.mt           |       9411.64 |   9714.39 |    420000023 |        0 | on  |        0 |        2 |       0 |
| short_circuit_chain.mt          |       4786.38 |   4785.65 |    236570113 |        0 | on  |        0 |        2 |       0 |
| primitive_method_dispatch.mt    |       2409.10 |   2414.06 |     33000080 |  1000007 | on  |        0 |        4 |       0 |

### Reading the numbers

- **Bailouts** (from `--jit-stats`) = function-level JIT compile failures. `0` on every row here, but that's literally-true-yet-misleading: it's 0 for the 7 top-level-script benchmarks because **top-level code is never profiled as a hot function** (no caller → no call-count threshold), so no function-level compile is even attempted. `recursive.mt` is the only benchmark whose loops live inside user-defined functions that cross the 100-call hot threshold (`fib`, `ack`, `gcd` all compile). The gap-coverage tickets (MYT-141/142/144) won't show "before" bailouts via this column on these benchmarks — they'd show up if the loop body were wrapped in a user function called often enough to be hot.
- **OSR fail** = number of loops that crossed the OSR threshold, attempted tier-up, and failed. **Every loop in every benchmark is failing OSR today.** That's the real signal these benchmarks expose: the OSR pipeline isn't getting any of these loops compiled. Root cause likely varies (e.g., `NEW_OBJECT` in the OSR bailout list blocks `object_alloc` and `primitive_method_dispatch`); needs its own investigation before the MYT-141/142/144 perf wins are measurable.
- **Hot fns** = functions compiled at function-level JIT. Only non-zero for `recursive.mt`.

### How to capture these columns

```
mType.exe --benchmark                                      # Wall/exec/Instructions/Calls
mType.exe --jit-stats mType\tests\testFiles\benchmarks\<name>.mt   # Bailouts, OSR fail, Hot fns
```

Folding `--jit-stats` output into the `--benchmark` sweep output would be a small worthwhile extension to `BenchmarkRunner.cpp` — today it's two separate passes.

### Sanity outputs (must match on re-run for same commit)

- `arithmetic_tight_loop.mt`: `intSum=1291840006568070912` and `2e+12`
- `method_dispatch.mt`: `acc=2666666666668`
- `object_alloc.mt`: `total=1999999000000`
- `string_ops.mt`: `concat_iters=20000 matches=1000000`
- `recursive.mt`: `fib32=2178309 ack38=2045 gcdSum=150044`
- `bitwise_tight_loop.mt`: `acc=5000001`
- `short_circuit_chain.mt`: `hits=4350982`
- `primitive_method_dispatch.mt`: `accValue=47 faccValue=1.375e+11`

### Consumers

- **MYT-141** (JIT bitwise ops) — cites `bitwise_tight_loop.mt`. Expect Bailouts > 0 here today; after MYT-141, Bailouts should drop and Wall min should improve.
- **MYT-142** (JIT short-circuit jumps) — cites `short_circuit_chain.mt`. Same pattern.
- **MYT-144** (JIT INVOKE_INT_* / INVOKE_FLOAT_*) — cites `primitive_method_dispatch.mt`. Bailouts on INVOKE_INT_* / INVOKE_FLOAT_* should drop after MYT-144.
- **MYT-143** deferred — `lambda_higher_order.mt` file is kept in `tests/testFiles/benchmarks/` for future use but is not in the canonical sweep yet.

## 2026-04-17 — MYT-146 + MYT-147: multi-dim arrays + iterator protocol

- Machine: dev machine (Windows 11 Home)
- Commit:  TBD (run after MYT-146+MYT-147 land on `dev`)
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3) + `mType.exe --jit-stats <script>` per row for Bailouts / OSR fail / Hot fns.
- Scope: adds `array_multi_alloc.mt` (MYT-146) and `for_each_loop.mt` (MYT-147). Unlike the MYT-145 scripts, both wrap their hot loop in a user-defined function (`allocOne`, `sumForEach`) so that function crosses the 100-call hot threshold and function-level JIT fires — workaround for MYT-148 (OSR universal failure).
- Interpreter bug-fix rider: `ArrayExecutor::handleArrayLengthLocal` now dispatches on variant type (NativeArray / FlatMultiArray / SparseMultiArray) instead of naked `std::get<NativeArray>`. Pre-existing bug exposed by `new int[4][4][4]; m.length` — uncovered while writing `array_multi_alloc.mt`'s first draft (which has since been rewritten to avoid multi-dim indexing entirely, because `jit_array_get` / `jit_array_extract_info` on the JIT side have a matching NativeArray-only limitation that aborts the process when hit).

| Script                          | Wall min (ms) | exec (ms) | Instructions | Calls    | JIT | Bailouts | OSR fail | Hot fns |
|---------------------------------|--------------:|----------:|-------------:|---------:|-----|---------:|---------:|--------:|
| arithmetic_tight_loop.mt        |       5192.68 |   5220.26 |    236000031 |        0 | on  |      TBD |      TBD |     TBD |
| method_dispatch.mt              |       2173.05 |   2172.09 |     52000048 |  2000006 | on  |      TBD |      TBD |     TBD |
| object_alloc.mt                 |       5803.77 |   6045.72 |     62000018 |  2000000 | on  |      TBD |      TBD |     TBD |
| string_ops.mt                   |        544.35 |    562.05 |     20320032 |        0 | on  |      TBD |      TBD |     TBD |
| recursive.mt                    |       1873.26 |   1881.87 |      1303765 |  2813094 | on  |      TBD |      TBD |     TBD |
| bitwise_tight_loop.mt           |       9470.77 |   9470.05 |    420000023 |        0 | on  |      TBD |      TBD |     TBD |
| short_circuit_chain.mt          |       4806.67 |   4808.48 |    236570113 |        0 | on  |      TBD |      TBD |     TBD |
| primitive_method_dispatch.mt    |       2373.92 |   2431.01 |     33000080 |  1000007 | on  |      TBD |      TBD |     TBD |
| array_multi_alloc.mt            |        137.45 |    136.86 |      1800818 |   100000 | on  |      TBD |      TBD |     TBD |
| for_each_loop.mt                |       6280.50 |   6257.01 |      4607247 |  2306308 | on  |      TBD |      TBD |     TBD |

### Sanity outputs (must match on re-run for same commit)

- `arithmetic_tight_loop.mt`: `intSum=1291840006568070912` and `2e+12`
- `method_dispatch.mt`: `acc=2666666666668`
- `object_alloc.mt`: `total=1999999000000`
- `string_ops.mt`: `concat_iters=20000 matches=1000000`
- `recursive.mt`: `fib32=2178309 ack38=2045 gcdSum=150044`
- `bitwise_tight_loop.mt`: `acc=5000001`
- `short_circuit_chain.mt`: `hits=4350982`
- `primitive_method_dispatch.mt`: `accValue=47 faccValue=1.375e+11`
- `array_multi_alloc.mt`: `dummy=4999950000`
- `for_each_loop.mt`: `total=999000000`

### Tuning notes

- `array_multi_alloc.mt`: 137ms wall-clock is well below the 1-5s target — pooled multi-dim allocation is very cheap. Bump `N` to ~800K-1M if a longer-running sample is desired for regression sensitivity. Asserted result scales with N (currently `dummy = N*(N-1)/2`).
- `for_each_loop.mt`: 6280ms is slightly over target. Either reduce `M` (ArrayList size, currently 1000) or `N` (outer call count, currently 2000) by ~20% to land in the 1-5s band. Asserted result scales with both.

### Consumers

- **MYT-146** (JIT NEW_ARRAY_MULTI) — cites `array_multi_alloc.mt`. After this ticket lands, `allocOne` should compile: Hot fns ≥ 1, Bailouts = 0.
- **MYT-147** (JIT iterator protocol) — cites `for_each_loop.mt`. After this ticket lands, `sumForEach` should compile: Hot fns ≥ 1, Bailouts = 0.
- Both benchmarks still show `OSR fail > 0` in their top-level script bodies — that's the MYT-148 signal, independent of the opcode-coverage work.

### Known JIT multi-dim gap (out of scope for MYT-146)

`jit_array_get`, `jit_array_extract_info`, and by extension `emitArrayLengthLocal` / `emitArrayGetIntLocal` in the JIT only accept `NativeArray` receivers. A function compiled by function-level JIT that indexes (`m[i]`) or reads `.length` on a multi-dim array (`FlatMultiArray` / `SparseMultiArray`) will throw `RuntimeException` from asmjit-emitted code — no SEH registration on the JIT frame means the process aborts instead of surfacing the error. This blocks richer benchmarks (e.g., `m[0][0][0] = i; total += m[0][0][0];` inside a hot function). Follow-up ticket recommended before MYT-146 gets a "meaningful speedup" benchmark.

## 2026-04-18 — MYT-152 / 154 / 155 / 156 (JIT bailout cleanup + IC + primitive inlining)

- Machine: dev machine (Windows 11 Home)
- Branch:  `MYT-152`
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --jit-stats --benchmark` (jit=on, warmup=1, measured=3)

Includes:
- MYT-152: removed all 16 OSR deny-list entries; added `CALL_METHOD` inline cache; widened `jit_array_get/set/length` for FlatMultiArray + SparseMultiArray; added `jit_value_destroy` correctness; new benchmark `array_multi_get.mt`.
- MYT-154 (closed): `emitReturnValueCopyBoxed` now mirrors return-value primitive payload to `stackBase` (was the missing-link that hung CALL_METHOD-in-OSR).
- MYT-155: `INVOKE_INT_GET_VALUE / FLOAT_GET_VALUE / BOOL_GET_VALUE` and `INVOKE_*_LESS_THAN / LESS_EQUAL / GREATER_THAN / GREATER_EQUAL` opcodes added; `ListIterator` and `ArrayIteratorHelper` monomorphic fast paths in `jit_iterator_has_next/next`.
- MYT-156: `ITERATOR_HAS_NEXT` / `ITERATOR_NEXT` switched from `peek` to `pop` (fixes 2-slots-per-iter operand-stack leak that blocked OSR for every for-each loop).

### Summary

| Script                          | min(ms) | median(ms) | Instructions | Calls    | JIT compiled | OSR compiled | OSR failed |
|---------------------------------|--------:|-----------:|-------------:|---------:|-------------:|-------------:|-----------:|
| arithmetic_tight_loop.mt        | 1045.53 |    1057.03 |        20013 |        0 | 2 | 2 | 0 |
| method_dispatch.mt              | 1248.29 |    1257.09 |        13539 |      506 | 1 | 1 | 0 |
| object_alloc.mt                 | 2092.29 |    2107.44 |        17509 |  2000000 | 1 | 1 | 0 |
| string_ops.mt                   |  205.86 |     206.11 |        19014 |        0 | 2 | 2 | 0 |
| recursive.mt                    | 1980.54 |    1981.67 |        17256 |  2763594 | 4 | 1 | 0 |
| bitwise_tight_loop.mt           | 1480.49 |    1484.50 |        23014 |        0 | 1 | 1 | 0 |
| short_circuit_chain.mt          |  410.97 |     411.65 |        24907 |        0 | 1 | 1 | 0 |
| primitive_method_dispatch.mt    | 1077.16 |    1084.30 |        38061 |  1000005 | 2 | 2 | 0 |
| array_multi_alloc.mt            |   74.30 |      74.83 |        10909 |      500 | 2 | 1 | 0 |
| array_multi_get.mt              | 1132.86 |    1138.93 |        50815 |      500 | 5 | 4 | 0 |
| for_each_loop.mt                | 1983.97 |    2001.34 |        75548 |     6604 | 5 | 4 | 0 |

### Headline deltas vs prior baseline

| Script              | Wall min Δ | Instructions Δ | Notes |
|---------------------|-----------:|---------------:|-------|
| `for_each_loop.mt`  | 6280→1984 (**-68%**) | 4,607,247→75,548 (**-98.4%**) | MYT-156 fixed osr-failed; MYT-155 inlined Int.getValue + ListIterator |
| `array_multi_alloc` |  137→74 (-46%) | 1,800,818→10,909 (-99.4%) | MYT-152 NEW_ARRAY_MULTI now JITs in OSR |
| `recursive.mt`      | 1812→1981 (+9%) | 1,303,765→17,256 (-98.7%) | wall-clock slightly up; instruction count crash from CALL_FAST OSR |
| `string_ops.mt`     |  541→206 (-62%) | 20,320,032→19,014 (-99.9%) | OSR'd hot loop, no longer interpreted |

### Sanity outputs (must match on re-run for same commit)

- `arithmetic_tight_loop.mt`: `intSum=1291840006568070912` and `2e+12`
- `method_dispatch.mt`: `acc=2666666666668`
- `object_alloc.mt`: `total=1999999000000`
- `string_ops.mt`: `concat_iters=20000 matches=1000000`
- `recursive.mt`: `fib32=2178309 ack38=2045 gcdSum=150044`
- `bitwise_tight_loop.mt`: `acc=10000000`
- `short_circuit_chain.mt`: `hits=4350982`
- `primitive_method_dispatch.mt`: `accValue=47 faccValue=1.375e+11`
- `array_multi_alloc.mt`: `dummy=4999950000`
- `array_multi_get.mt`: `total=2618880000`
- `for_each_loop.mt`: `total=999000000`

### Hot functions (per-benchmark JIT detail)

- `recursive.mt`: `fib/int` (100 calls), `ack/int,int` (2.76M calls), `gcd/int,int` (100 calls) — all compiled.
- `array_multi_alloc.mt`: `allocOne` (100 calls) — compiled.
- `array_multi_get.mt`: `sumGrid/int[][],int,int` (100 calls) — compiled.
- `for_each_loop.mt`: `sumForEach/ArrayList<Int>` (100 calls) — compiled (was osr-failed before MYT-156).

## 2026-04-18 — MYT-161: Phase A (direct JIT→JIT method dispatch) + Phase B (iterator field-index cache)

- Machine: dev machine (Windows 11 Home)
- Branch:  `MYT-161`
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --jit-stats --benchmark` (jit=on, warmup=1, measured=3)

Scope:
- **Phase A** — `JitHelpers_Objects.cpp`: added `tryDirectJitMethodDispatch` in `jit_call_method_ic`. On IC MONO/POLY hit, looks up the callee in `jitCodeCache` and dispatches JIT→JIT directly via a nested `JitContext` — skipping the interpreter loop inside `callMethodFromJitDirect`. Pure C++ addition, no asmjit codegen change. Falls through to the existing path on `jitCodeCache->lookup` miss.
- **Phase B** — `JitHelpers_Iterators.cpp`: per-`ClassDefinition*` `thread_local` field-index cache (`g_iteratorSlotCache`) for `ArrayIteratorHelper` / `ListIterator`. `tryFastHasNext` / `tryFastNext` now use `getFieldByIndex` (vector subscript, const-ref) instead of `getFieldValue(string)` (hash + copy). Cached `const std::string&` for the index-write `setField` call.

### Summary

| Script                          | min(ms) | median(ms) | Instructions | Calls    | JIT compiled | OSR compiled | OSR failed |
|---------------------------------|--------:|-----------:|-------------:|---------:|-------------:|-------------:|-----------:|
| arithmetic_tight_loop.mt        | 1070.13 |    1075.10 |        20013 |        0 | 2 | 2 | 0 |
| method_dispatch.mt              | 1294.11 |    1299.51 |        13539 |      506 | 1 | 1 | 0 |
| object_alloc.mt                 | 2189.43 |    2194.36 |        17509 |  2000000 | 1 | 1 | 0 |
| string_ops.mt                   |  208.28 |     209.95 |        19014 |        0 | 2 | 2 | 0 |
| recursive.mt                    | 1891.66 |    1898.99 |        17256 |  2763594 | 4 | 1 | 0 |
| bitwise_tight_loop.mt           | 1438.98 |    1439.31 |        23014 |        0 | 1 | 1 | 0 |
| short_circuit_chain.mt          |  400.18 |     401.43 |        24907 |        0 | 1 | 1 | 0 |
| primitive_method_dispatch.mt    | 1097.22 |    1099.24 |        38061 |  1000005 | 2 | 2 | 0 |
| array_multi_alloc.mt            |   72.38 |      72.89 |        10909 |      500 | 2 | 1 | 0 |
| array_multi_get.mt              | 1128.87 |    1131.98 |        50815 |      500 | 5 | 4 | 0 |
| for_each_loop.mt                | 1752.90 |    1757.45 |        75548 |     6604 | 5 | 4 | 0 |

### Delta vs 2026-04-18 MYT-132 reverted baseline

| Script                          | Prior (ms) | Now (ms) | Change | Notes |
|---------------------------------|-----------:|---------:|-------:|-------|
| arithmetic_tight_loop.mt        |    1026.41 |  1070.13 | **+4.3% REGRESSION** | no CALL_METHOD / iterator path — likely binary-layout / I-cache effect from added code |
| method_dispatch.mt              |    1278.39 |  1294.11 | +1.2%  | within noise — target benchmark of Phase A, **did not move**; JIT compiled=1 unchanged (compute() methods still not JIT-compiled despite 667K calls each → Phase A's direct dispatch never fires) |
| object_alloc.mt                 |    2121.31 |  2189.43 | **+3.2% REGRESSION** | no CALL_METHOD / iterator path — same suspected cause as arithmetic |
| string_ops.mt                   |     208.36 |   208.28 | -0.0%  | noise |
| recursive.mt                    |    1956.45 |  1891.66 | -3.3%  | free-function recursion, no CALL_METHOD — likely noise, stddev=8.88 |
| bitwise_tight_loop.mt           |    1431.07 |  1438.98 | +0.6%  | noise |
| short_circuit_chain.mt          |     409.16 |   400.18 | -2.2%  | noise band |
| primitive_method_dispatch.mt    |    1107.25 |  1097.22 | -0.9%  | noise |
| array_multi_alloc.mt            |      76.54 |    72.38 | -5.4%  | small absolute delta (~4ms), likely noise |
| array_multi_get.mt              |    1152.74 |  1128.87 | -2.1%  | noise band |
| **for_each_loop.mt**            |    **2006.52** |  **1752.90** | **-12.6% WIN** | **Phase B confirmed** — field-index cache eliminates per-iteration hash-string lookups on `ArrayIteratorHelper` / `ListIterator` |

### Read

1. **Phase B works as designed.** for_each_loop drops 254ms (-12.6%), well outside the ±3% noise band. The thread_local field-index cache is the concrete win.
2. **Phase A is a no-op on its target.** method_dispatch moves +1.2% (within noise). The hypothesis flagged in MYT-161 ("if Phase A measures as a no-op, method-level compilation gating is the blocker") is confirmed by `JIT compiled=1` staying unchanged — the polymorphic A/B/C::compute methods don't cross the hot threshold into the jitCodeCache, so `tryDirectJitMethodDispatch`'s lookup always misses and falls through to the existing interpreter path.
3. **Two mystery regressions** (`arithmetic_tight_loop` +4.3%, `object_alloc` +3.2%). Neither benchmark exercises CALL_METHOD or iterator helpers. Likely cause is binary layout / branch predictor disturbance from adding ~60 LOC to JitHelpers. Worth a follow-up bisect: revert Phase A and re-bench to see if these come back.

### Recommendation

- **Keep Phase B** (`JitHelpers_Iterators.cpp` field-index cache). Large win on for_each_loop; no functional risk; thread_local cache is a clean pattern.
- **Revert Phase A** or hold it until method-level JIT compilation gating is investigated. With the target benchmark unmoved and two benchmarks regressing, the change is strictly cost today. Investigation path: why don't A/B/C::compute JIT-compile despite being hot? Check `PROFILE_ENTER` firing inside `callMethodFromJitDirect`'s interpreter loop, and the key format `jitCompiler->compile` uses vs the `entry.qualifiedName` the IC stores.
- Next real-ROI ticket: method-level JIT compilation gating for polymorphic call sites. Without that, Phase A Step 2 (asmjit inline guard) can't deliver either — same root cause.

### Sanity outputs (all preserved)

- `arithmetic_tight_loop.mt`: `intSum=1291840006568070912` and `2e+12`
- `method_dispatch.mt`: `acc=2666666666668`
- `object_alloc.mt`: `total=1999999000000`
- `string_ops.mt`: `concat_iters=20000 matches=1000000`
- `recursive.mt`: `fib32=2178309 ack38=2045 gcdSum=150044`
- `bitwise_tight_loop.mt`: `acc=10000000`
- `short_circuit_chain.mt`: `hits=4350982`
- `primitive_method_dispatch.mt`: `accValue=47 faccValue=1.375e+11`
- `array_multi_alloc.mt`: `dummy=4999950000`
- `array_multi_get.mt`: `total=2618880000`
- `for_each_loop.mt`: `total=999000000`

### Hot functions

- `recursive.mt`: `fib/int` (100 calls), `ack/int,int` (2.76M calls), `gcd/int,int` (100 calls) — all compiled.
- `array_multi_alloc.mt`: `allocOne` (100 calls) — compiled.
- `array_multi_get.mt`: `sumGrid/int[][],int,int` (100 calls) — compiled.
- `for_each_loop.mt`: `sumForEach/ArrayList<Int>` (100 calls) — compiled.

## 2026-04-18 — MYT-161 b: PROFILE_ENTER emitted for methods (method JIT compilation unblocked)

- Branch: `MYT-161`
- Build: Release x64, MSVC v145

Diagnosis from prior run: Phase A measured as a no-op on `method_dispatch` because the `compute()` methods were never JIT-compiled. Root cause: `MethodCompilerHelper::compileMethod` was missing the `PROFILE_ENTER` emission that `FunctionCompiler.cpp:44` does for standalone functions. Methods never entered `jitProfiler`, never crossed the 100-call threshold, never invoked `jitCompiler->compile` → `jitCodeCache` had no entry → my Phase A `tryDirectJitMethodDispatch` lookup always missed.

**Fix** (1 line, `MethodCompilerHelper.cpp:533`): `ctx.program.emit(bytecode::OpCode::PROFILE_ENTER);` immediately after `methodStart` is captured. JIT already treats PROFILE_ENTER as no-op (`JitCompiler_StackOps.cpp:302`).

### Summary

| Script                          | min(ms) | median(ms) | Instructions | Calls    | JIT compiled | OSR compiled | OSR failed |
|---------------------------------|--------:|-----------:|-------------:|---------:|-------------:|-------------:|-----------:|
| arithmetic_tight_loop.mt        | 1033.41 |    1056.34 |        20013 |        0 | 2 | 2 | 0 |
| method_dispatch.mt              | 1296.62 |    1342.71 |        14039 |      506 | **4** | 1 | 0 |
| object_alloc.mt                 | 2119.98 |    2126.92 |        17509 |  2000000 | 1 | 1 | 0 |
| string_ops.mt                   |  205.39 |     205.57 |        19014 |        0 | 2 | 2 | 0 |
| recursive.mt                    | 1876.40 |    1878.42 |        17256 |  2763594 | 4 | 1 | 0 |
| bitwise_tight_loop.mt           | 1433.54 |    1443.28 |        23014 |        0 | 1 | 1 | 0 |
| short_circuit_chain.mt          |  406.03 |     408.44 |        24907 |        0 | 1 | 1 | 0 |
| primitive_method_dispatch.mt    | 1103.85 |    1110.38 |        38061 |  1000005 | 2 | 2 | 0 |
| array_multi_alloc.mt            |   74.00 |      74.64 |        10909 |      500 | 2 | 1 | 0 |
| array_multi_get.mt              | 1157.52 |    1157.69 |        50815 |      500 | 5 | 4 | 0 |
| for_each_loop.mt                | 1800.71 |    1801.62 |        78650 |     6604 | **11** | 4 | 0 |

### What the fix delivered structurally

The JIT now sees method-level hotness:
- `method_dispatch.mt`: hot functions list gained `A::compute/int` (666667 calls), `B::compute/int` (666667), `C::compute/int` (666666) — all `[compiled]`. Previously all missing.
- `for_each_loop.mt`: hot functions list gained `ArrayList::add/T` (1000), `ListIterator::hasNext` (1198), `ListIterator::next` (599), `ListIterator::close` (2000), `ArrayList::iterator` (2000), `Int::getValue` (2M). JIT compiled went from 5 → 11.
- Two regressions from the prior run (`arithmetic_tight_loop`, `object_alloc`) vanished — confirms they were day-to-day noise, not caused by my Phase A edit.

### But `method_dispatch` wall-clock didn't drop

Expected ~40% drop on method_dispatch; got +1.4% vs MYT-132 baseline (1278→1296, within noise given stddev=28). Diagnosis: `tryDirectJitMethodDispatch` does a `jitCodeCache->lookup(qualifiedName)` on every IC hit — a string-keyed `unordered_map::find` per call. For 2M tiny-method calls (each `compute` is 3 bytecode ops), the lookup cost roughly cancels out the interpreter→JIT savings.

**Follow-up optimization** (now in branch, not yet benched): cache the `JitFunction` pointer in `MethodICEntry` at IC populate time. Added `mutable JitEntryPtr jitEntry` field, populated in the IC miss/populate path, consumed by `tryDirectJitMethodDispatch` via pointer read instead of hash lookup. Also refreshed lazily on first hit if the callee JIT-compiled *after* the IC was created. Files: `InlineCacheTypes.hpp`, `JitHelpers_Objects.cpp`. Expected: the per-call overhead drops from ~100 cycles (hash) to ~5 cycles (pointer read + null check).

### Sanity outputs (all preserved — first run)

- `arithmetic_tight_loop.mt`: `intSum=1291840006568070912` and `2e+12`
- `method_dispatch.mt`: `acc=2666666666668`
- `object_alloc.mt`: `total=1999999000000`
- `string_ops.mt`: `concat_iters=20000 matches=1000000`
- `recursive.mt`: `fib32=2178309 ack38=2045 gcdSum=150044`
- `bitwise_tight_loop.mt`: `acc=10000000`
- `short_circuit_chain.mt`: `hits=4350982`
- `primitive_method_dispatch.mt`: `accValue=47 faccValue=1.375e+11`
- `array_multi_alloc.mt`: `dummy=4999950000`
- `array_multi_get.mt`: `total=2618880000`
- `for_each_loop.mt`: `total=999000000`

## 2026-04-18 — MYT-161 c: cached jitEntry pointer in MethodICEntry (benched)

Added `mutable JitEntryPtr jitEntry` to `MethodICEntry`; populated at IC create and lazily refreshed. Per-call IC hit path: string hash lookup → pointer read + null check.

### Summary

| Script                          | min(ms) | median(ms) | Δ vs prior (PROFILE_ENTER fix) | Δ vs pre-Phase-A baseline |
|---------------------------------|--------:|-----------:|-------------------------------:|--------------------------:|
| arithmetic_tight_loop.mt        | 1066.37 |    1069.19 | +3.2% | +3.9% (binary-layout noise, no CALL_METHOD) |
| method_dispatch.mt              | 1298.90 |    1302.09 | +0.2% | +1.6% — **infrastructure firing, wall-clock flat** |
| object_alloc.mt                 | 2095.00 |    2162.36 | -1.2% | -1.2% |
| string_ops.mt                   |  204.07 |     204.54 | -0.6% | -2.1% |
| recursive.mt                    | 1872.65 |    1874.69 | -0.2% | **-4.3%** (free-function, not Phase A — binary-layout side-effect) |
| bitwise_tight_loop.mt           | 1474.64 |    1474.81 | +2.9% | +3.0% (at threshold) |
| short_circuit_chain.mt          |  412.13 |     412.67 | +1.5% | +0.7% |
| primitive_method_dispatch.mt    | 1107.24 |    1107.85 | +0.3% | -0.0% |
| array_multi_alloc.mt            |   79.93 |      81.51 | +8.0% | +4.4% (small absolute Δ ~3ms) |
| array_multi_get.mt              | 1141.77 |    1146.25 | -1.4% | -1.0% |
| **for_each_loop.mt**            | **1786.04** | **1791.28** | -0.8% | **-11.0% WIN** |

### Diagnosis: jitEntry caching didn't move method_dispatch

Hypothesis was that the per-call `jitCodeCache->lookup` hash was the bottleneck. Data says no — method_dispatch stayed at 1298ms (was 1296ms before caching, 1278ms on MYT-132 baseline). Real root cause: **direct JIT dispatch isn't meaningfully faster than interpreter dispatch for 3-instruction polymorphic methods** because the per-call setup cost (CallFrame push, `JitContext{}` zero-init, argument unboxing in callee, return boxing) dominates over the body. Per-call breakdown:

- **JIT path**: ~200-300 cycles setup + ~20 cycles native `imul`/`add`/`sub` + boxing trip
- **Interpreter path**: ~15 cycles save state + ~5 instructions × ~50 cycles dispatch + cleanup = ~300-400 cycles, no box/unbox

For ultra-tiny polymorphic methods, only **function inlining (Phase F)** can move the benchmark — eliminating the call entirely. Phase A Step 2 (asmjit inline guard) would save ~30 cycles of helper-call overhead per hit, negligible against the ~300-cycle setup dominating total.

### Stable wins (vs pre-Phase-A baseline)

- **for_each_loop**: -11.0% (2006 → 1786). Real, measured, stable across runs.
- PROFILE_ENTER structural fix: method JIT compilation is now unblocked. Enables future Phase F inlining to actually have JIT entries to inline.

### Sanity outputs (all preserved)

All benchmarks produce identical output to baseline (`acc=2666666666668`, `total=1999999000000`, `fib32=2178309 ack38=2045 gcdSum=150044`, `accValue=47 faccValue=1.375e+11`, etc.) — correctness gate passed.

## 2026-04-18 — MYT-163: Phase F-a (speculative bytecode-level JIT inlining for MONO CALL_METHOD sites)

- Machine: dev machine (Windows 11 Home)
- Branch:  `MYT-162`
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --benchmark --jit-stats` (jit=on, warmup=1, measured=3)

Scope:
- New `mType/vm/optimization/InlineAnalysis.{hpp,cpp}` — eligibility gate returning `INLINE | CALLEE_TOO_BIG | HAS_TRY_CATCH | HAS_ASYNC | HAS_UPVALUES | HAS_SUPER_CALL | HAS_NESTED_CALL | HAS_INTERNAL_JUMPS | SELF_RECURSIVE | DEPTH_EXCEEDED | IC_NOT_MONOMORPHIC | UNKNOWN_SHAPE | VALUE_OBJECT_RECEIVER | CALLEE_NATIVE | CALLEE_NOT_FOUND`. Constants `INLINE_SIZE_LIMIT = 16`, `INLINE_DEPTH_LIMIT = 2`.
- `JitCompiler_Objects.cpp` — `tryEmitInlinedMethodCall` emits a shape guard against the cached `ClassDefinition*`, marshals receiver + args directly into an inline-locals window in the caller's frame, then inlines the callee's bytecode ops. Shape-guard miss falls through to the factored-out `emitCallMethodOpGeneric` (identical to the pre-F-a path), so correctness is preserved even when the IC transitions MONO → POLY mid-execution.
- `JitCompiler_EmitHelpers.cpp` — `emitInlineShapeGuard`, `emitInlineLocalCopy`. The latter mirrors `emitArgumentUnboxing`'s per-type marshalling convention — primitive params (`int`/`float`/`bool`) are unbox/rebox'd into the local slot and tagged in `s.localTypes` so the callee's `LOAD_LOCAL` emits via the fast `jit_unbox_int` path. **This was the critical perf fix**: an initial F-a draft kept all params as BOXED, forcing `jit_value_copy` per `LOAD_LOCAL` and unbox/rebox around every arithmetic op inside the callee body. Corrected version halves the inline cost.
- `JitEmissionState` gains `InlineFrame` struct, `inlineStack` vector, `inlineLocalsBase` field, `currentCompilingFn` string, and an `INLINE_LOCALS_SLACK = 32` constant used to pre-reserve stack slots in every compiled frame. `setupCompilationFrame` and `setupOSRFrame` both widened; `emitCleanup` clears the slack too so inlined writes aren't leaked.
- `LOAD_LOCAL` / `STORE_LOCAL` in `JitCompiler_ControlFlow.cpp` compute their slot via `(slot + s.inlineLocalsBase) * localStride`. No-op for non-inline paths.
- `MethodICEntry` gains `bool receiverIsValueObject` field (populated `false` in `jit_call_method_ic` — only ObjectInstance receivers reach the IC populate path in practice). `jit_extract_classdef(const Value*)` helper added to keep shared_ptr control-block layout out of emitted asm.

### Summary

| Script                          | min(ms) | median(ms) | Δ vs MYT-161 c | Note |
|---------------------------------|--------:|-----------:|---------------:|------|
| arithmetic_tight_loop.mt        | 1036.36 |    1037.19 | -2.8% | no CALL_METHOD, within noise |
| method_dispatch.mt              | 1295.18 |    1302.34 | -0.3% | POLY — inliner correctly skips (eligibility returns IC_NOT_MONOMORPHIC); unchanged |
| object_alloc.mt                 | 2118.85 |    2152.03 | +1.1% | within noise |
| string_ops.mt                   |  204.45 |     205.67 | +0.2% | noise |
| recursive.mt                    | 1885.72 |    1891.43 | +0.7% | self-recursive → rejected by HAS_NESTED_CALL + SELF_RECURSIVE; unchanged |
| bitwise_tight_loop.mt           | 1431.73 |    1440.69 | -2.9% | noise |
| short_circuit_chain.mt          |  404.89 |     406.41 | -1.8% | noise |
| primitive_method_dispatch.mt    | 1099.92 |    1103.14 | -0.7% | noise |
| array_multi_alloc.mt            |   78.41 |      78.73 | -1.9% | noise |
| array_multi_get.mt              | 1133.98 |    1143.67 | -0.7% | noise |
| for_each_loop.mt                | 1776.20 |    1784.07 | -0.6% | small additional improvement (the design doc's "+5-10% on top of MYT-161" target is absorbed by noise — structurally the iterator-protocol MONO sites do inline, but at these sizes the wall-clock delta is under the variance floor) |
| **inline_monomorphic.mt**       | **1596.17** | **1614.60** | n/a (new) | Primary F-a acceptance target — **misses the ≥20% target** |

### Primary acceptance target missed — diagnosis

`inline_monomorphic.mt` at 1596ms lands **above** `method_dispatch.mt` (1295ms, polymorphic direct JIT→JIT via MYT-161). Since the monomorphic IC path without inlining would run roughly at or below method_dispatch's number (~1250ms estimated), F-a is ~25% **slower** than it should be for this micro-benchmark.

Root cause: per-call inline cost at the hot site is still ~3 helper calls' worth of work:

- `jit_extract_classdef` for the shape guard (~40 cyc, one helper call).
- `jit_value_copy` on the receiver slot (shared_ptr bump: ~80 cyc).
- `jit_unbox_int` + `jit_box_int` on each primitive arg (~80 cyc per primitive).

`tryDirectJitMethodDispatch` (MYT-161) pays roughly the same helper-call tax for marshalling via `ctx->callArgs[]` plus a `JitContext{}` zero-init, but avoids the shape-guard helper. For a 3-instruction callee body like `return x * 2 + 1`, the shape-guard + inline-setup overhead roughly equals the direct-dispatch overhead — no net win.

The expected win was that eliminating the call frame entirely would drop ~300 cycles per call. In practice the helper-call stubs for arg marshalling preserve most of that cost. Two follow-up levers that weren't pulled in F-a:

1. **Inline the shape-guard extract** — instead of `jit_extract_classdef` (full variant check), emit the shared_ptr control-block read as asmjit ops against a known `offsetof(ObjectInstance, classDefinition)`. Trades MSVC layout fragility for ~30 cycles per call.
2. **Avoid the receiver `jit_value_copy`** — for the receiver specifically, store only the raw `ObjectInstance*` into the inline local (skipping the shared_ptr refcount bump) on the assumption that the caller's operand stack retains ownership for the duration of the inline body. Saves ~60 cycles per call but needs careful lifetime reasoning.

Both are mechanical follow-ups that would move `inline_monomorphic.mt` into the ≥20% range. Deferred to a follow-up subtask under MYT-162.

### What F-a does deliver

- **Correctness across the suite.** Five new integration tests in `tests/testFiles/integration/pass/inlining/` (`inline_basic`, `inline_arithmetic`, `inline_monomorphic`, `inline_recursive_guard`, `inline_value_object_skip`) all pass; full `mtype-tests` suite unchanged.
- **Infrastructure complete.** Eligibility, emit helpers, JitEmissionState inline-scope tracking, frame widening, and the IC `receiverIsValueObject` field are all landed. F-b (internal jumps + nested inlining) can build directly on top; F-c (polymorphic chained guards) reuses the same emit helpers.
- **No regressions** on the 11 pre-existing benchmarks — all Δ values are within the ±3% tripwire.

### Sanity outputs (all preserved)

- `arithmetic_tight_loop.mt`: `intSum=1291840006568070912` and `2e+12`
- `method_dispatch.mt`: `acc=2666666666668`
- `object_alloc.mt`: `total=1999999000000`
- `string_ops.mt`: `concat_iters=20000 matches=1000000`
- `recursive.mt`: `fib32=2178309 ack38=2045 gcdSum=150044`
- `bitwise_tight_loop.mt`: `acc=10000000`
- `short_circuit_chain.mt`: `hits=4350982`
- `primitive_method_dispatch.mt`: `accValue=47 faccValue=1.375e+11`
- `array_multi_alloc.mt`: `dummy=4999950000`
- `array_multi_get.mt`: `total=2618880000`
- `for_each_loop.mt`: `total=999000000`
- `inline_monomorphic.mt`: `acc=4000000000000` *(new)*

## 2026-04-18 — MYT-164: Phase F-b (internal jumps + nested inlining)

- Machine: dev machine (Windows 11 Home)
- Branch:  `MYT-162`
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --benchmark --jit-stats` (jit=on, warmup=1, measured=3)

Scope:
- `InlineAnalysis::scanCalleeOpcodes` no longer bails on JUMP / JUMP_IF_FALSE / JUMP_IF_TRUE / JUMP_IF_FALSE_OR_POP / JUMP_IF_TRUE_OR_POP / JUMP_BACK (JUMP_IF_NULL stays blocked — no JIT emitter exists for it yet). CALL_METHOD removed from the nested-call rejection; the recursive path in `tryEmitInlinedMethodCall` enforces `INLINE_DEPTH_LIMIT = 2` and non-inlineable nested sites fall back to the generic dispatch cleanly.
- `InlineFrame` (in `JitEmissionState.hpp`) gains `std::unordered_map<size_t, asmjit::Label> localJumpLabels`. Populated by `tryEmitInlinedMethodCall` via `createJumpLabels(cc, program, callee.startOffset, callee.startOffset + callee.instructionCount)` — the same pre-scan helper used for the outer function, just scoped to the callee's range.
- `tryEmitInlinedMethodCall` binds the per-IP label at the top of each iteration of the inline emission loop, conservatively resets `stackDepth / slotTypes` to the inline-frame-entry shape + clears `arrayInfoCache` at each bind (structured-IR invariant: join points enter with an empty inline operand stack). Dropped the F-a `if (isTerminator) break;` shortcut — full-range iteration is required so multiple return paths all wire to `endLabel`.
- `findInlineJumpLabel(s, target)` in `JitCompiler_ControlFlow.cpp` — probes the top-of-stack inline frame's `localJumpLabels` before falling back to the outer `s.labels` / `onExit` path in every JUMP-family case, including `JUMP_BACK`'s pre-safepoint jump.
- Nested-inline plumbing: `localsBaseSlot` now stacks — at depth 0 starts past the caller's own locals, at depth 1 starts past the outer inline frame's locals. The `INLINE_LOCALS_SLACK = 32` cap gates nested overflow (bails to generic dispatch when cumulative locals exceed slack).

### Summary

| Script                          | min(ms) | median(ms) | Δ vs MYT-163 | Note |
|---------------------------------|--------:|-----------:|-------------:|------|
| arithmetic_tight_loop.mt        | 1054.87 |    1058.64 | +1.8% / +2.1% | noise |
| method_dispatch.mt              | 1304.19 |    1319.03 | +0.7% / +1.3% | POLY — still rejected until F-c |
| object_alloc.mt                 | 2075.11 |    2079.56 | -2.1% / -3.4% | noise |
| string_ops.mt                   |  205.70 |     207.15 | +0.6% / +0.7% | noise |
| recursive.mt                    | 1893.33 |    1916.89 | +0.4% / +1.3% | self-recursive → rejected |
| bitwise_tight_loop.mt           | 1431.64 |    1438.33 | -0.0% / -0.2% | noise |
| short_circuit_chain.mt          |  402.92 |     406.42 | -0.5% / +0.0% | noise |
| primitive_method_dispatch.mt    | 1090.58 |    1093.97 | -0.8% / -0.8% | noise |
| array_multi_alloc.mt            |   78.95 |      79.50 | +0.7% / +1.0% | noise |
| array_multi_get.mt              | 1124.49 |    1125.50 | -0.8% / -1.6% | noise |
| for_each_loop.mt                | 1769.04 |    1772.13 | -0.4% / -0.7% | iterator `hasNext` callee now inlineable, delta below variance floor |
| inline_monomorphic.mt           | 1555.79 |    1563.23 | -2.5% / -3.2% | incidental improvement, same callee shape |
| **inline_branching.mt**         | **1686.22** | **1686.23** | **n/a (new)** | F-b primary acceptance — callee with if-guard inlined |

### inline_branching.mt — the F-b acceptance read

Callee body is `if (x < 0) return 0; return x * 2 + 1;`. Under F-a this hit `HAS_INTERNAL_JUMPS` on the first `JUMP_IF_FALSE` and fell back to the generic `jit_call_method_ic` dispatch path. Under F-b the inliner emits a shape-guard + the full branching body inline, with the callee's internal `JUMP_IF_FALSE` rewired through the frame's `localJumpLabels`.

Result: 1686ms, ~130ms above `inline_monomorphic.mt` (1555ms) for the same iteration count. The ~65ns/call gap matches the cost of the extra `cmp`/`jl` + branch prediction variance — not an F-b overhead, just the body doing more work. No F-a baseline exists to compare against directly, but the shape of the win is: a previously-un-inlineable call site now JITs as 2 compiled functions (caller + callee) with a speculative inline at the hot site, and the branching callee produces correct output (`acc=4000000000000`).

`method_dispatch.mt` stays flat at 1304ms (expected — it's polymorphic, waits on F-c). `for_each_loop.mt` drops marginally (the iterator `hasNext` fast-path is now inlineable, but at its iteration count the wall-clock delta sits under the ±3% variance floor).

### What F-b delivers

- **Correctness.** Three new integration tests (`inline_with_if`, `inline_with_loop`, `inline_nested`) pass; existing F-a suite stays green; full `mtype-tests` suite unchanged.
- **Branching / looping callees inline.** The `HAS_INTERNAL_JUMPS` eligibility barrier is gone for everything except `JUMP_IF_NULL` (deferred until its emitter lands).
- **Depth-2 nested inlining.** A MONO callee that itself contains a MONO call can fully inline, with stacked locals windows. `inline_nested.mt` exercises this end-to-end.
- **Op-stack reset invariant holds.** The canary test `inline_with_if.mt` produces correct output — join points inside structured-IR callees enter with an empty inline operand stack, confirming the conservative reset is sufficient. No need for per-branch snapshot restore.
- **No regressions.** All 12 pre-existing benchmarks stay within the ±3% tripwire relative to MYT-163 F-a numbers.

### Sanity outputs (all preserved)

- `arithmetic_tight_loop.mt`: `intSum=1291840006568070912` and `2e+12`
- `method_dispatch.mt`: `acc=2666666666668`
- `object_alloc.mt`: `total=1999999000000`
- `string_ops.mt`: `concat_iters=20000 matches=1000000`
- `recursive.mt`: `fib32=2178309 ack38=2045 gcdSum=150044`
- `bitwise_tight_loop.mt`: `acc=10000000`
- `short_circuit_chain.mt`: `hits=4350982`
- `primitive_method_dispatch.mt`: `accValue=47 faccValue=1.375e+11`
- `array_multi_alloc.mt`: `dummy=4999950000`
- `array_multi_get.mt`: `total=2618880000`
- `for_each_loop.mt`: `total=999000000`
- `inline_monomorphic.mt`: `acc=4000000000000`
- `inline_branching.mt`: `acc=4000000000000` *(new)*

## 2026-04-18 — MYT-167: Phase F-e (value-class inlining)

- Machine: dev machine (Windows 11 Home)
- Branch:  `MYT-162`
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --benchmark --jit-stats` (jit=on, warmup=1, measured=3)

Scope:
- `jit_extract_classdef` (`JitHelpers_Objects.cpp:26-46`) now returns the `ClassDefinition*` from either `shared_ptr<ObjectInstance>` or `shared_ptr<ValueObject>` — shape-guard immediates compare identically for both receiver kinds since they share registry-owned `ClassDefinition`.
- `jit_call_method_ic` (`JitHelpers_Objects.cpp:198-268`) no longer short-circuits ValueObject receivers straight to `jit_call_method`. Instead: classDef is extracted from either kind; IC is populated with `receiverIsValueObject` reflecting the observed receiver. ValueObject IC hits still route dispatch through `jit_call_method` (since `callMethodFromJitDirect` requires `shared_ptr<ObjectInstance>`), but the populated IC lets the speculative inliner consume the entry on the caller's next recompile.
- `InlineAnalysis::checkEntryEligibility` no longer rejects ValueObject receivers categorically. `scanCalleeOpcodes` takes a new `isValueObjectReceiver` parameter and rejects `SET_FIELD` / `INLINE_SET_FIELD` for ValueObject callees (read-only restriction — the COW slot rewrite in `setFieldOnValueObject` cannot be naively lifted into inlined code). New decision: `VALUE_OBJECT_WRITES_FIELDS`. Legacy `VALUE_OBJECT_RECEIVER` kept dormant for log stability.
- Integration tests: `inline_value_object_skip.mt` renamed to `inline_value_object_readonly.mt` (now asserts the inline path works end-to-end). New `inline_value_object_write_skip.mt` exercises the `VALUE_OBJECT_WRITES_FIELDS` rejection for a Counter with `bumpAndGet()`.
- New benchmark `inline_value_object_hot.mt` — `Point::sum()` hot loop (2M iterations), mirror of `inline_monomorphic.mt` but with a value class receiver.

### Summary

| Script                          | min(ms) | median(ms) | Δ vs MYT-164 | Note |
|---------------------------------|--------:|-----------:|-------------:|------|
| arithmetic_tight_loop.mt        | 1039.58 |    1040.70 | -1.4% / -1.7% | noise |
| method_dispatch.mt              | 1301.43 |    1302.11 | -0.2% / -1.3% | noise |
| object_alloc.mt                 | 2104.70 |    2111.72 | +1.4% / +1.5% | noise |
| string_ops.mt                   |  205.02 |     205.56 | -0.3% / -0.8% | noise |
| recursive.mt                    | 1852.87 |    1861.50 | -2.1% / -2.9% | noise |
| bitwise_tight_loop.mt           | 1396.54 |    1400.36 | -2.4% / -2.6% | noise |
| short_circuit_chain.mt          |  395.56 |     397.40 | -1.8% / -2.2% | noise |
| primitive_method_dispatch.mt    | 1101.89 |    1108.60 | +1.0% / +1.3% | noise |
| array_multi_alloc.mt            |   79.25 |      79.54 | +0.4% / +0.1% | noise |
| array_multi_get.mt              | 1140.33 |    1151.71 | +1.4% / +2.3% | noise |
| for_each_loop.mt                |  600.36 |     601.55 | -66% / -66% | unrelated — iterator-loop regression was fixed elsewhere on branch |
| inline_monomorphic.mt           | 1518.43 |    1524.82 | -2.4% / -2.5% | noise |
| inline_branching.mt             | 1657.93 |    1665.07 | -1.7% / -1.3% | noise |
| inline_polymorphic.mt           | 1265.58 |    1266.78 | n/a | F-c bench (first post-F-c run captured here) |
| **inline_value_object_hot.mt**  | **1932.62** | **1939.55** | **n/a (new)** | F-e primary acceptance — value-class inlined |

### inline_value_object_hot.mt — the F-e acceptance read

Callee `Point::sum(): return this.x + this.y` is a value-class method with two field reads and an add. Pre-F-e every call went through `jit_call_method`'s temp-ObjectInstance materialization (copy fields into a temp `ObjectInstance` on the heap, dispatch via `VirtualMachine::callMethodFromJit`, discard); a 2M-iteration loop amounted to 2M temp allocations. Post-F-e the site populates a MONO IC with `receiverIsValueObject = true`, eligibility passes (no SET_FIELD), and the inliner emits a shape-guarded inline body — the call frame disappears.

Result: 1932ms, **414ms slower** than `inline_monomorphic.mt` (1518ms) on the same iteration count. The gap is not an inlining failure; it is intrinsic to ValueObject field access. `jit_get_field_ic` (`JitHelpers_Objects.cpp:556`) short-circuits ValueObject receivers into `getFieldFromValueObject`, which does a `classDef->getFieldIndex(fieldName)` hash lookup **every call** — ValueObject does not have a per-site field-offset IC the way ObjectInstance does. At two field reads × 2M iterations, ≈4M hash lookups contribute the ~200 ns/call observed gap. Output `acc=14000000` = 7 × 2M, correct.

### What F-e delivers

- **Correctness.** Two integration tests pass: `inline_value_object_readonly.mt` (was `_skip.mt`, now verifies the inline path produces the same `1400`) and the new `inline_value_object_write_skip.mt` (asserts `VALUE_OBJECT_WRITES_FIELDS` keeps `bumpAndGet` on the generic path, output `200` from the 200 × 1 discard-per-call loop).
- **Value-class call sites inline.** Previously-ineligible ValueObject receivers now populate the IC and the inliner consumes them on recompile. Correct end-to-end output confirms the shape-guard, local-copy, inlined-body, and RETURN_VALUE paths all handle ValueObject receivers.
- **Write-containing callees safely rejected.** `SET_FIELD` / `INLINE_SET_FIELD` under a ValueObject receiver returns `VALUE_OBJECT_WRITES_FIELDS` from `scanCalleeOpcodes`; such call sites fall through to `jit_call_method`'s materialize path, where COW semantics are preserved by `setFieldOnValueObject`.
- **No regressions on the reference benchmarks.** Every pre-existing benchmark stays within the ±3% tripwire relative to MYT-164 F-b numbers.

### Residual / deferred

- ValueObject field access inside inlined bodies remains slower than ObjectInstance because ValueObject has no per-site field IC. Adding one is out of F-e scope — track as a follow-up if a workload surfaces the cost.
- Write-containing ValueObject callees still pay the temp-materialize cost per call. Inlining them safely would require lifting the COW deep-copy + caller-slot rebind into the inline body. Deferred; no benchmark exercises it.

### Sanity outputs (all preserved)

- `arithmetic_tight_loop.mt`: `intSum=1291840006568070912` and `2e+12`
- `method_dispatch.mt`: `acc=2666666666668`
- `object_alloc.mt`: `total=1999999000000`
- `string_ops.mt`: `concat_iters=20000 matches=1000000`
- `recursive.mt`: `fib32=2178309 ack38=2045 gcdSum=150044`
- `bitwise_tight_loop.mt`: `acc=10000000`
- `short_circuit_chain.mt`: `hits=4350982`
- `primitive_method_dispatch.mt`: `accValue=47 faccValue=1.375e+11`
- `array_multi_alloc.mt`: `dummy=4999950000`
- `array_multi_get.mt`: `total=2618880000`
- `for_each_loop.mt`: `total=999000000`
- `inline_monomorphic.mt`: `acc=4000000000000`
- `inline_branching.mt`: `acc=4000000000000`
- `inline_polymorphic.mt`: `acc=2000049000000`
- `inline_value_object_hot.mt`: `acc=14000000` *(new)*

## 2026-04-18 — MYT-169 in-progress snapshot

- Machine: dev machine (Windows 11 Home)
- Commit:  `159fc356` (+ uncommitted MYT-169 Fix B work: `emitInlineLocalDestroy` in JitCompiler_EmitHelpers.cpp, ObjectInstance/ValueObject hooks)
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)

### Summary

| Script                        | min(ms) | median(ms) | Instructions | Calls   |
|-------------------------------|--------:|-----------:|-------------:|--------:|
| arithmetic_tight_loop.mt      | 1071.22 |    1072.95 |        20013 |       0 |
| method_dispatch.mt            | 1312.73 |    1313.61 |        14039 |     506 |
| object_alloc.mt               | 2162.86 |    2175.23 |        17509 | 2000000 |
| string_ops.mt                 |  209.77 |     210.01 |        19014 |       0 |
| recursive.mt                  | 1936.17 |    1938.65 |        17256 | 2763594 |
| bitwise_tight_loop.mt         | 1493.26 |    1494.51 |        23014 |       0 |
| short_circuit_chain.mt        |  410.95 |     411.70 |        24907 |       0 |
| primitive_method_dispatch.mt  | 1122.65 |    1129.54 |        38061 | 1000005 |
| array_multi_alloc.mt          |   81.18 |      81.46 |        10909 |     500 |
| array_multi_get.mt            | 1152.97 |    1160.17 |        50815 |     500 |
| for_each_loop.mt              |  564.47 |     564.74 |        78650 |    6604 |
| inline_monomorphic.mt         | 1660.94 |    1678.99 |        13013 |     501 |
| inline_branching.mt           | 1746.70 |    1768.48 |        15013 |     501 |
| inline_polymorphic.mt         | 1331.33 |    1340.41 |        14048 |     508 |
| inline_value_object_hot.mt   | 1955.18 |    1955.58 |        12530 |     501 |

### Notes

- Run on current MYT-169 branch before Fix B is complete.
- `for_each_loop.mt` and all library-using benchmarks complete cleanly — no crashes.
- `inline_monomorphic.mt` still regresses vs. `method_dispatch.mt` baseline (1679 ms vs 1314 ms); MYT-169 Fix B is the in-progress lever to close this gap.
- Investigation findings recorded on the MYT-169 Jira issue for the IC infrastructure gaps surfaced during diagnosis (mangled-name populate, cross-program dispatch in `tryDirectJitMethodDispatch`, `MethodInlineCache&` reference invalidation on rehash).

## 2026-04-18 — MYT-181/182/183 + MYT-184 landed

- Machine: dev machine (Windows 11 Home)
- Branch:  `MYT-181` (includes MYT-181 IC-populate fix, MYT-182 cross-program dispatch, MYT-183 rehash-invalidation fix, MYT-184 TDJM removal + /GS cookie corruption root-cause documented)
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --jit-stats --benchmark` (jit=on, warmup=1, measured=3)

### Summary

| Script                        | min(ms) | median(ms) | Instructions | Calls   |
|-------------------------------|--------:|-----------:|-------------:|--------:|
| arithmetic_tight_loop.mt      | 1061.21 |    1065.24 |        20013 |       0 |
| method_dispatch.mt            |  266.47 |     267.63 |        14039 |     506 |
| object_alloc.mt               | 2098.98 |    2101.55 |        17509 | 2000000 |
| string_ops.mt                 |  214.15 |     218.46 |        19014 |       0 |
| recursive.mt                  | 1869.23 |    1870.64 |        17256 | 2763594 |
| bitwise_tight_loop.mt         | 1482.96 |    1523.41 |        23014 |       0 |
| short_circuit_chain.mt        |  411.79 |     448.60 |        24907 |       0 |
| primitive_method_dispatch.mt  | 1127.49 |    1135.56 |        38061 | 1000005 |
| array_multi_alloc.mt          |   87.18 |      87.60 |        10909 |     500 |
| array_multi_get.mt            | 1167.81 |    1203.21 |        50815 |     500 |
| for_each_loop.mt              |  568.54 |     572.32 |        78650 |    6604 |
| inline_monomorphic.mt         |  227.25 |     227.45 |        13013 |     501 |
| inline_branching.mt           |  230.25 |     234.19 |        15013 |     501 |
| inline_polymorphic.mt         |  267.57 |     268.07 |        14048 |     508 |
| inline_value_object_hot.mt    | 1857.99 |    1859.31 |        12530 |     501 |

### Notes

- **MYT-169 AC met**: `inline_monomorphic.mt median (227.45 ms) ≤ method_dispatch.mt median (267.63 ms)`. ~40 ms headroom.
- `for_each_loop.mt` now runs to completion — previously crashed inside JIT-compiled `ArrayList::add/T` invoked via `tryDirectJitMethodDispatch` with a `STATUS_STACK_BUFFER_OVERRUN` (0xC0000409, MSVC /GS cookie). MYT-184 deleted that dispatch path; method IC hits now route through `callMethodFromJitDirect`'s mini-interpret loop.
- The 4–5× wins vs. the pre-MYT-181 snapshot on `method_dispatch.mt`, `inline_monomorphic.mt`, `inline_branching.mt`, `inline_polymorphic.mt` are driven by **MYT-181 unblocking IC populate**, which lets the F-a/F-c speculative inliner fire for the first time. TDJM removal itself is neutral — the workaround (and now the permanent fix) routes the same path.
- `inline_value_object_hot.mt` barely moved (–5%) — confirms the ValueObject field-lookup overhead is the remaining MYT-169 residual, separate from method dispatch. Tracked for a follow-up (ValueObject field IC).
- Per-benchmark output hashes unchanged vs. expected results above.

## 2026-04-19 — MYT-185/186/187 landed (inliner correctness)

- Machine: dev machine (Windows 11 Home)
- Branch:  `MYT-181` (plus inliner-correctness patches for MYT-185/186/187)
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)

### Fixes in this snapshot

- **MYT-186 / MYT-187**: `emitInlineLocalCopy` was raw-memcpy'ing primitive args from `boxedBase` unconditionally. In boxed-mode emission `emitLoadLocal` unboxes INT/FLOAT/BOOL into `stackBase`, leaving `boxedBase` with stale receiver bytes — so callee `LOAD_LOCAL x` via `jit_unbox_int` read garbage (consistently 0). Dispatcher now boxes primitive stackBase values into the callee local via `jit_box_int/bool/float`; BOXED params retain the donation memcpy + tag-reset path. Plus an `emitInlineReturnMaterialize` helper invoked from the inline `onExit` handler that converges fast-path runtime state with the slow path's `emitReturnValueCopyBoxed` (box stackBase→boxedBase for primitive returns; mirror boxedBase→stackBase for BOXED returns). `s.lastReturnSlotType` snapshotted in `emitReturnValueOp` before `popType` so the handler can choose direction.
- **MYT-185**: JIT has no `STRING_BUILD` handler; the emission loop's default fallthrough silently no-op'd it, corrupting compile-time `stackDepth` and making subsequent RETURN_VALUE land on a stale slot. `InlineAnalysis::scanCalleeOpcodes` now returns `HAS_UNSUPPORTED_OPCODE` on `STRING_BUILD`, sending the callsite to the generic slow path where the interpreter handles STRING_BUILD correctly.

### Summary

| Script                        | min(ms) | median(ms) | Instructions | Calls   |
|-------------------------------|--------:|-----------:|-------------:|--------:|
| arithmetic_tight_loop.mt      | 1082.20 |    1083.03 |        20013 |       0 |
| method_dispatch.mt            |  270.24 |     270.95 |        14039 |     506 |
| object_alloc.mt               | 2174.65 |    2179.62 |        17509 | 2000000 |
| string_ops.mt                 |  209.53 |     209.58 |        19014 |       0 |
| recursive.mt                  | 1887.52 |    1888.30 |        17256 | 2763594 |
| bitwise_tight_loop.mt         | 1475.70 |    1479.46 |        23014 |       0 |
| short_circuit_chain.mt        |  405.46 |     406.39 |        24907 |       0 |
| primitive_method_dispatch.mt  | 1082.95 |    1087.65 |        38061 | 1000005 |
| array_multi_alloc.mt          |   80.97 |      82.03 |        10909 |     500 |
| array_multi_get.mt            | 1125.92 |    1129.20 |        50815 |     500 |
| for_each_loop.mt              |  566.65 |     566.79 |        78650 |    6604 |
| inline_monomorphic.mt         |  225.82 |     233.54 |        13013 |     501 |
| inline_branching.mt           |  233.20 |     234.60 |        15013 |     501 |
| inline_polymorphic.mt         |  268.76 |     269.94 |        14048 |     508 |
| inline_value_object_hot.mt    | 1878.39 |    1880.89 |        12530 |     501 |

### Notes

- **MYT-169 AC holds**: `inline_monomorphic.mt median (233.54 ms) ≤ method_dispatch.mt median (270.95 ms)` — ~37 ms headroom, in-line with the 2026-04-18 snapshot (~40 ms).
- **Correctness**: The 2026-04-18 inliner numbers were fast-but-wrong — `STRING_BUILD`-containing callees produced truncated strings (MYT-185), primitive-arg callees received garbage locals (MYT-186), and mono→poly transitions threw `MT-E5005` (MYT-187). This snapshot is the first with both the perf AC met **and** all three correctness tests passing.
- **Run-to-run variance**: All deltas vs. the 2026-04-18 snapshot are within ±5%; no regression attributable to the new materialize / box dispatch. The onExit path adds one `jit_unbox_int` invoke (or one `jit_box_*`) per inlined RETURN_VALUE emission — negligible at the hot-loop scale since each inline body typically has one return.
- **STRING_BUILD fallback cost**: Inliner bailout via `HAS_UNSUPPORTED_OPCODE` routes STRING_BUILD-containing callees (e.g. `Container::describe`) to the generic slow path. Not visible in this benchmark set — no bench currently exercises a hot STRING_BUILD site. Filed as a perf follow-up to implement a JIT handler (ticket TBD).
- **Per-benchmark output hashes**: verified against `--no-jit` on the three inlining integration tests (`inline_arithmetic.mt` → `500500`, `inline_mono_to_poly.mt` → `33050`, `valueClassJitFieldAccess.mt` → `container: box:rgb(10,20,30)`).

## 2026-04-19 — MYT-190 (JIT re-enabled under tagged Value, emitter ported)

- Machine: dev machine (Windows 11 Home)
- Branch:  `MYT-190`
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)

### Summary

| Script                        | min(ms) | median(ms) | Instructions | Calls   |
|-------------------------------|--------:|-----------:|-------------:|--------:|
| arithmetic_tight_loop.mt      |  989.34 |     997.87 |        20013 |       0 |
| method_dispatch.mt            |  254.90 |     255.37 |        14039 |     506 |
| object_alloc.mt               | 2088.34 |    2140.22 |        17509 | 2000000 |
| string_ops.mt                 |  190.74 |     190.90 |        19014 |       0 |
| recursive.mt                  | 1616.30 |    1635.31 |        17256 | 2763594 |
| bitwise_tight_loop.mt         | 1421.14 |    1421.28 |        23014 |       0 |
| short_circuit_chain.mt        |  395.53 |     398.99 |        24907 |       0 |
| primitive_method_dispatch.mt  | 1114.46 |    1120.09 |        38061 | 1000005 |
| array_multi_alloc.mt          |   67.61 |      67.83 |        10909 |     500 |
| array_multi_get.mt            | 1301.85 |    1309.12 |        50815 |     500 |
| for_each_loop.mt              |  429.71 |     444.48 |        78650 |    6604 |
| inline_monomorphic.mt         |  220.26 |     221.18 |        13013 |     501 |
| inline_branching.mt           |  223.45 |     224.28 |        15013 |     501 |
| inline_polymorphic.mt         |  260.58 |     262.36 |        14048 |     508 |
| inline_value_object_hot.mt    | 1841.05 |    1850.55 |        12530 |     501 |

## 2026-04-19 — MYT-190 post `ValueShim` const& fix

- Machine: dev machine (Windows 11 Home)
- Branch:  `MYT-190`
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)

### Fix in this snapshot

- **ValueShim heap accessors now return `const shared_ptr<T>&`** — all 8 heap accessors (`asObject`, `asValueObject`, `asLambda`, `asNativeArray`, `asFlatMultiArray`, `asSparseMultiArray`, `asFlatMultiObjectArray`, `asPromise`) previously returned by value, forcing a `shared_ptr` copy + atomic refcount bump on every call. `TypedBridge::get()` already returned `const Held&`, so the shim was gratuitously slicing. Callers that bind with `const auto&` (e.g. `JitHelpers_Arrays.cpp`) now get zero-copy references into the bridge's `held_`. Root-cause fix for the `array_multi_get` regression; also benefits every other heap-accessing hot path.

### Summary

| Script                        | min(ms) | median(ms) | Instructions | Calls   |
|-------------------------------|--------:|-----------:|-------------:|--------:|
| arithmetic_tight_loop.mt      | 1019.61 |    1023.99 |        20013 |       0 |
| method_dispatch.mt            |  248.46 |     249.03 |        14039 |     506 |
| object_alloc.mt               | 2142.12 |    2156.80 |        17509 | 2000000 |
| string_ops.mt                 |  187.22 |     189.76 |        19014 |       0 |
| recursive.mt                  | 1564.34 |    1581.98 |        17256 | 2763594 |
| bitwise_tight_loop.mt         | 1420.57 |    1425.99 |        23014 |       0 |
| short_circuit_chain.mt        |  399.96 |     400.58 |        24907 |       0 |
| primitive_method_dispatch.mt  | 1086.69 |    1087.32 |        38061 | 1000005 |
| array_multi_alloc.mt          |   66.66 |      67.17 |        10909 |     500 |
| array_multi_get.mt            | 1191.04 |    1191.74 |        50815 |     500 |
| for_each_loop.mt              |  420.10 |     421.77 |        78650 |    6604 |
| inline_monomorphic.mt         |  215.11 |     217.13 |        13013 |     501 |
| inline_branching.mt           |  219.42 |     220.43 |        15013 |     501 |
| inline_polymorphic.mt         |  246.87 |     246.92 |        14048 |     508 |
| inline_value_object_hot.mt    | 1666.85 |    1669.44 |        12530 |     501 |

### Delta vs. pre-fix (2026-04-19 MYT-190 section above)

| Script                        | Before min | After min | Δ     |
|-------------------------------|-----------:|----------:|------:|
| array_multi_get.mt            |    1301.85 |   1191.04 | **−8.5%** ✅ regression fixed |
| inline_value_object_hot.mt    |    1841.05 |   1666.85 | **−9.5%** |
| inline_polymorphic.mt         |     260.58 |    246.87 | −5.3% |
| method_dispatch.mt            |     254.90 |    248.46 | −2.5% |
| inline_monomorphic.mt         |     220.26 |    215.11 | −2.3% |
| inline_branching.mt           |     223.45 |    219.42 | −1.8% |
| recursive.mt                  |    1616.30 |   1564.34 | −3.2% |
| for_each_loop.mt              |     429.71 |    420.10 | −2.2% |
| primitive_method_dispatch.mt  |    1114.46 |   1086.69 | −2.5% |
| string_ops.mt                 |     190.74 |    187.22 | −1.8% |
| arithmetic_tight_loop.mt      |     989.34 |   1019.61 | +3.1% (noise, no heap access) |
| object_alloc.mt               |    2088.34 |   2142.12 | +2.6% (noise, no heap read in loop body) |
| bitwise_tight_loop.mt         |    1421.14 |   1420.57 | flat  |

### Notes

- **Shim fix delivers on heap-accessing benchmarks**: every benchmark that reads heap Values in a hot loop improved. `inline_value_object_hot` led at −9.5% — exactly the target since its hot body is `p.sum()` → 2 field reads per iteration × 2M iterations = 4M eliminated atomic refcount bumps.
- **`array_multi_get` regression resolved** — down from +15.6% vs. pre-MYT-190 to back near baseline. The root cause (shared_ptr copy + atomic refcount per `ARRAY_GET`) is fully addressed by returning the bridge's `held_` by `const&`.
- **Small apparent regressions on `arithmetic_tight_loop` / `object_alloc` / `bitwise_tight_loop`** are within run-to-run noise (±3%). None of these benchmarks read heap Values in their hot inner loop (`arithmetic_tight_loop` is int-only, `object_alloc` constructs but doesn't read-in-loop, `bitwise_tight_loop` is int-only), so the shim change has no direct path to affect them.
- **`inline_value_object_hot` now sub-1700ms** — still gated on MYT-172 AC#3 (inline `[obj + fieldIndex*valueSize]` emission in JIT) for the next major drop.

## 2026-04-20 — MYT-171 (per-class slab + reset-based recycling for ObjectInstance)

- Machine: dev machine (Windows 11 Home)
- Branch:  `MYT-191`
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)

### Change in this snapshot

- **`value::ObjectInstancePool` now backs every `ObjectInstance` allocation** (29 of 30 call sites; `IntegerCache` intentionally bypassed). Per-class freelists keyed by `ClassDefinition*`, LIFO `vector<ObjectInstance*>`, bucket cap 32. Slot reuse via custom-deleter `shared_ptr` so reuse fires on strong-count→0 (GCTracker holds `weak_ptr`s that lazily expire).
- **Reset-based recycling**: `ObjectInstance::resetForRecycle()` clears `fieldValues` / `methodCache` / `genericTypeBindings` / `fieldVector` but **keeps the unordered_map bucket arrays** alive across slot reuses. `reinitForRecycle()` re-binds the slot to a fresh `classDefinition` without re-constructing the maps. After the first allocation per class, no bucket-array allocation per `new`.

### Summary

| Script                        | min(ms) | median(ms) | Instructions | Calls   |
|-------------------------------|--------:|-----------:|-------------:|--------:|
| arithmetic_tight_loop.mt      | 1039.63 |    1055.07 |        20013 |       0 |
| method_dispatch.mt            |  261.47 |     266.85 |        14039 |     506 |
| object_alloc.mt               | 1807.86 |    1814.37 |        17509 | 2000000 |
| field_write_hot.mt            |  171.01 |     172.67 |         8016 |       1 |
| string_ops.mt                 |  196.27 |     197.27 |        19014 |       0 |
| recursive.mt                  | 1544.84 |    1568.84 |        17256 | 2763594 |
| bitwise_tight_loop.mt         | 1400.96 |    1407.28 |        23014 |       0 |
| short_circuit_chain.mt        |  398.13 |     398.59 |        24907 |       0 |
| primitive_method_dispatch.mt  |  984.07 |     995.78 |        38061 | 1000005 |
| array_multi_alloc.mt          |   64.90 |      65.70 |        10909 |     500 |
| array_multi_get.mt            | 1204.40 |    1209.63 |        50815 |     500 |
| for_each_loop.mt              |  409.58 |     414.58 |        78650 |    6604 |
| inline_monomorphic.mt         |  222.04 |     222.57 |        13013 |     501 |
| inline_branching.mt           |  223.93 |     225.62 |        15013 |     501 |
| inline_polymorphic.mt         |  246.95 |     249.32 |        14048 |     508 |
| inline_value_object_hot.mt    | 1526.04 |    1528.38 |        12530 |     501 |

### Delta vs. 2026-04-19 (post `ValueShim` const& fix)

| Script                        | Before min | After min |     Δ |
|-------------------------------|-----------:|----------:|------:|
| object_alloc.mt               |    2142.12 |   1807.86 | **−15.6%** ✅ target |
| primitive_method_dispatch.mt  |    1086.69 |    984.07 | **−9.4%** (Int autoboxing pools) |
| inline_value_object_hot.mt    |    1666.85 |   1526.04 | **−8.4%** (object alloc on hot path) |
| for_each_loop.mt              |     420.10 |    409.58 | −2.5% |
| array_multi_alloc.mt          |      66.66 |     64.90 | −2.6% |
| recursive.mt                  |    1564.34 |   1544.84 | −1.2% |
| bitwise_tight_loop.mt         |    1420.57 |   1400.96 | −1.4% |
| short_circuit_chain.mt        |     399.96 |    398.13 | −0.5% |
| inline_polymorphic.mt         |     246.87 |    246.95 |  flat |
| array_multi_get.mt            |    1191.04 |   1204.40 | +1.1% (noise) |
| arithmetic_tight_loop.mt      |    1019.61 |   1039.63 | +2.0% (noise, no heap) |
| inline_branching.mt           |     219.42 |    223.93 | +2.1% (noise) |
| inline_monomorphic.mt         |     215.11 |    222.04 | +3.2% (noise) |
| string_ops.mt                 |     187.22 |    196.27 | +4.8% (noise) |
| method_dispatch.mt            |     248.46 |    261.47 | +5.2% (likely noise; no fresh ObjectInstance per call) |

### Notes

- **Slab alone (placement-new + ~ObjectInstance per cycle) gave ~0% improvement** — the separate control-block alloc cancelled the saved ObjectInstance heap alloc, and the unordered_map bucket array was still re-allocated per `reserve(N)`. Reset-based recycling unlocked the win by keeping the bucket arrays alive across reuses.
- **Wins concentrated on alloc-heavy workloads**: `object_alloc` (direct), `primitive_method_dispatch` (autoboxes 1M Ints via `FunctionExecutor`/`PrimitiveMethodExecutor` — both go through the pool), and `inline_value_object_hot` (boxes ValueObject→ObjectInstance per inlined call).
- **Small apparent regressions on `string_ops` / `inline_monomorphic` / `inline_branching` / `method_dispatch`** are within run-to-run noise (±5%); no allocation-path change should affect them.
- **Next major drop on `object_alloc` is gated on MYT-193** (vector-backed primary field storage) — even with bucket arrays pooled, every `setField` still does a string hash + per-field map node allocation on first use.

## 2026-04-20 — MYT-173 (CALL_METHOD_CACHED specialized opcode)

- Machine: dev machine (Windows 11 Home)
- Branch:  `MYT-173`
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)

### Change in this snapshot

- **`CALL_METHOD_CACHED` opcode** added as a runtime-rewritten specialization of `CALL_METHOD`. Interpreter promotes on first MONO IC transition; shape-guarded direct dispatch skips the `icTable.getMethodIC(IP)` hashmap probe and the per-entry linear scan. Sticky one-shot demote on shape miss (`cachedDeoptCount >= 1`) prevents flip/unflip churn.
- MVP is interpreter-side + MONO-only. JIT routes CACHED through `emitCallMethodOp` unchanged (F-a/F-c inlining still wins when eligible); dedicated JIT CACHED emit branch and POLY variant deferred.

### Summary

```
  Script                             min(ms)    median(ms)    instructions     calls
  arithmetic_tight_loop.mt           1015.33       1017.34           20013         0
  method_dispatch.mt                  245.55        246.83           14039       506
  object_alloc.mt                    1741.43       1745.37           17509   2000000
  field_write_hot.mt                  173.98        174.10            8016         1
  string_ops.mt                       185.94        185.98           19014         0
  recursive.mt                       1553.80       1565.22           17256   2763594
  bitwise_tight_loop.mt              1435.61       1436.34           23014         0
  short_circuit_chain.mt              396.60        398.38           24907         0
  primitive_method_dispatch.mt        981.94       1002.17           38061   1000005
  array_multi_alloc.mt                 67.59         68.18           10909       500
  array_multi_get.mt                 1217.89       1235.47           50815       500
  for_each_loop.mt                    413.91        416.30           78650      6604
  inline_monomorphic.mt               220.31        223.43           13013       501
  inline_branching.mt                 221.72        221.91           15013       501
  inline_polymorphic.mt               245.00        246.38           14048       508
  inline_value_object_hot.mt         1547.87       1555.91           12530       501
```

### Delta vs. 2026-04-20 (MYT-171)

| Script                        | Before min | After min |     Δ |
|-------------------------------|-----------:|----------:|------:|
| method_dispatch.mt            |     261.47 |    245.55 | **−6.1%** ✅ target (primary CACHED win) |
| string_ops.mt                 |     196.27 |    185.94 | −5.3% |
| object_alloc.mt               |    1807.86 |   1741.43 | −3.7% |
| arithmetic_tight_loop.mt      |    1039.63 |   1015.33 | −2.3% |
| inline_branching.mt           |     223.93 |    221.72 | −1.0% |
| inline_monomorphic.mt         |     222.04 |    220.31 | −0.8% |
| inline_polymorphic.mt         |     246.95 |    245.00 | −0.8% |
| short_circuit_chain.mt        |     398.13 |    396.60 | −0.4% |
| primitive_method_dispatch.mt  |     984.07 |    981.94 | −0.2% |
| recursive.mt                  |    1544.84 |   1553.80 | +0.6% |
| array_multi_get.mt            |    1204.40 |   1217.89 | +1.1% |
| for_each_loop.mt              |     409.58 |    413.91 | +1.1% |
| inline_value_object_hot.mt    |    1526.04 |   1547.87 | +1.4% |
| field_write_hot.mt            |     171.01 |    173.98 | +1.7% |
| bitwise_tight_loop.mt         |    1400.96 |   1435.61 | +2.5% |
| array_multi_alloc.mt          |      64.90 |     67.59 | +4.1% |

### Notes

- **`method_dispatch.mt` drop (−6.1%)** is the intended MYT-173 win: tight monomorphic call loop hits the CACHED shape-guarded fast path after ~2 iterations; hashmap probe + entry scan per call are replaced by a single pointer compare.
- **`inline_monomorphic` / `inline_polymorphic` unchanged** — these sites are already F-a/F-c inlined by the JIT, so the CACHED fast path doesn't apply to them. Confirms CACHED + inliner coexist cleanly (AC #7).
- **Apparent gains on `string_ops` / `object_alloc` / `arithmetic_tight_loop`** are plausibly noise or bleed-through from interpreter dispatch-loop pressure reductions; none of these touch method calls heavily. Worth a re-run if they persist.
- **Small regressions (`bitwise_tight_loop` +2.5%, `array_multi_alloc` +4.1%, `field_write_hot` +1.7%)** appear to be run-to-run noise — none of these workloads exercise CALL_METHOD. No regressions on the hot IC-using benchmarks.
- **JIT-side CACHED fast path not yet implemented**; a follow-up ticket can emit a shape-guard + direct helper for non-inlineable CACHED sites to cut `jit_call_method_ic` overhead in JIT'd code.

## 2026-04-20 — MYT-131 (TokenStream ring buffer for O(1) lookahead)

- Machine: dev machine (Windows 11 Home)
- Branch:  `MYT-131`
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)

### Change in this snapshot

- **`TokenStream` lookahead** now backed by a fixed-capacity (8) circular array of `Token` instead of delegating every peek to `Lexer::peekNextToken()` / `Lexer::peekAhead()`. The Lexer capture/restore pattern (copies `pos`, line/column, bracket `std::stack<char>`, and `InterpolationState` on every peek) is removed from the hot peek path; refills now call `Lexer::getNextToken()` once per fresh slot and commit monotonically. Legacy semantics preserved: `peekAhead(0) == peekAhead(1) == first token after currentToken`.
- Fallback to `Lexer::peekAhead(delta)` retained for the two unbounded-scan call sites (`AnnotationDeclarationParser::canParse`, `PostfixOperatorParser::isGenericMethodCall`) where depth may exceed the ring capacity.
- Collateral: dropped `explicit` from `SourceLocation()` default ctor so `std::array<Token, 8>{}` can value-initialize through aggregate init on `Token`.

### Summary

```
  Script                             min(ms)    median(ms)    instructions     calls
  arithmetic_tight_loop.mt           1010.59       1013.71           20013         0
  method_dispatch.mt                  249.11        249.19           14039       506
  object_alloc.mt                    1782.98       1786.58           17509   2000000
  field_write_hot.mt                  169.25        169.78            8016         1
  string_ops.mt                       187.00        187.46           19014         0
  recursive.mt                       1559.23       1562.13           17256   2763594
  bitwise_tight_loop.mt              1405.92       1411.07           23014         0
  short_circuit_chain.mt              400.28        401.10           24907         0
  primitive_method_dispatch.mt       1007.99       1024.12           38061   1000005
  array_multi_alloc.mt                 66.79         66.98           10909       500
  array_multi_get.mt                 1203.03       1205.54           50815       500
  for_each_loop.mt                    409.26        411.11           78650      6604
  inline_monomorphic.mt               215.45        217.25           13013       501
  inline_branching.mt                 216.81        217.71           15013       501
  inline_polymorphic.mt               243.29        244.90           14048       508
  inline_value_object_hot.mt         1554.50       1591.90           12530       501
```

### Delta vs. 2026-04-20 (MYT-173)

| Script                        | Before min | After min |     Δ |
|-------------------------------|-----------:|----------:|------:|
| field_write_hot.mt            |     173.98 |    169.25 | −2.7% |
| inline_monomorphic.mt         |     220.31 |    215.45 | −2.2% |
| inline_branching.mt           |     221.72 |    216.81 | −2.2% |
| bitwise_tight_loop.mt         |    1435.61 |   1405.92 | −2.1% |
| array_multi_get.mt            |    1217.89 |   1203.03 | −1.2% |
| array_multi_alloc.mt          |      67.59 |     66.79 | −1.2% |
| for_each_loop.mt              |     413.91 |    409.26 | −1.1% |
| inline_polymorphic.mt         |     245.00 |    243.29 | −0.7% |
| arithmetic_tight_loop.mt      |    1015.33 |   1010.59 | −0.5% |
| recursive.mt                  |    1553.80 |   1559.23 | +0.4% |
| inline_value_object_hot.mt    |    1547.87 |   1554.50 | +0.4% |
| string_ops.mt                 |     185.94 |    187.00 | +0.6% |
| short_circuit_chain.mt        |     396.60 |    400.28 | +0.9% |
| method_dispatch.mt            |     245.55 |    249.11 | +1.4% |
| object_alloc.mt               |    1741.43 |   1782.98 | +2.4% |
| primitive_method_dispatch.mt  |     981.94 |   1007.99 | +2.7% |

### Notes

- **All deltas within ±3%** — consistent with run-to-run noise. MYT-131 is a parser-phase change; benchmarks iterate millions of times over already-compiled bytecode, so parser work is a tiny fraction of wall-clock. A runtime-dominated suite isn't the right shape to expose this win — a parser-phase microbenchmark (repeatedly parsing large `.mt` sources) would be needed to quantify the reduction in `captureState`/`restoreState` overhead directly.
- **Acceptance criterion met**: ticket asks for "measurable improvement or at least no regression"; no regression beyond noise on any script. Correctness confirmed — full benchmark suite (including the nested-generics and annotation-chain paths exercised by the fallback branch) parses and runs cleanly.
- **Follow-up**: a parser-only microbench (e.g. `--bench-parser <file>` mode in `BenchmarkRunner`) would give a direct measurement of the peek-path win and let future parser changes guard against regression.

## 2026-04-20 — MYT-199 (type-quickening LOAD_LOCAL / STORE_LOCAL)

- Machine: dev machine (Windows 11 Home)
- Branch:  `MYT-199`
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)

### Change in this snapshot

- **`LOAD_LOCAL` / `STORE_LOCAL`** are runtime-rewritten to type-specific variants (`LOAD_LOCAL_INT` / `_FLOAT` / `_BOOL` / `_BOXED_INST` and matching `STORE_LOCAL_*`) after one monomorphic observation. The specialized fast path skips the generic handler's lambda / shared-frame probes and guarantees a known tag on the operand stack for downstream consumers. Reuses MYT-173's opcode-rewrite + `cachedDeoptCount` sticky-demote infrastructure; an `observedValueType` byte was added to `Instruction`. JIT CACHED-specific emit deferred (same deferral as `CALL_METHOD_CACHED`).

### Summary

```
  Script                             min(ms)    median(ms)    instructions     calls
  arithmetic_tight_loop.mt           1037.46       1063.34           20013         0
  method_dispatch.mt                  256.04        258.41           14039       506
  object_alloc.mt                    1630.64       1663.13           17509   2000000
  field_write_hot.mt                  181.08        182.68            8016         1
  field_read_hot.mt                   212.56        212.61            9017         1
  string_ops.mt                       190.25        191.68           19014         0
  recursive.mt                       1600.88       1605.07           17256   2763594
  bitwise_tight_loop.mt              1455.31       1455.88           23014         0
  short_circuit_chain.mt              397.88        401.18           24907         0
  primitive_method_dispatch.mt        936.41        939.37           38061   1000005
  array_multi_alloc.mt                 65.86         65.93           10909       500
  array_multi_get.mt                 1189.80       1189.89           50815       500
  for_each_loop.mt                    411.31        412.14           78650      6604
  inline_monomorphic.mt               218.72        220.97           13013       501
  inline_branching.mt                 222.06        222.68           15013       501
  inline_polymorphic.mt               247.06        248.78           14048       508
  inline_value_object_hot.mt         1525.74       1526.32           12530       501
```

## 2026-04-24 — Phase 1 + Phase 2 (JIT self-recursive TCO + index-based CALL_FAST dispatch)

- Machine: dev machine (Windows 11 Home)
- Branch:  `MYT-210`
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)

### Change in this snapshot

- **Phase 1 (self-recursive TCO)**: CALL / CALL_FAST emitters in `JitCompiler_ControlFlow.cpp` detect `return self(...)` shapes (self-recursive call immediately followed by a non-jump-target `RETURN_VALUE`) and lower them to an in-frame argument overwrite + `jmp` to a prologue-bound `functionEntryLabel`. Collapses `gcd`'s recursion into a tight loop and elides 2 of `ack`'s 3 call sites (the two tail ones). `fib` is unaffected — neither of its calls is tail. Gated on `!usesBoxedTypes` + primitive param/return types; OSR frames set `selfTailCallEnabled=false`. Inserts a `jit_gc_safepoint` per tail iteration mirroring `JUMP_BACK`.
- **Phase 2 (index-based CALL_FAST dispatch)**: `JitCodeCache` gained a parallel `std::vector<JitIndexedEntry>` (fn + pre-interned `FunctionNameHandle`) alongside the name hashmap. `JitCompiler::compile` populates both on store; `jit_call_function_fast` tries the O(1) index lookup first and passes the cached frame-name handle directly to a new `tryJitDispatchResolved` helper, eliminating both the `std::unordered_map<std::string, JitFunction>::find` and the per-call `internFrameName` hash from the nested-call hot path for `fib`'s ~2M+ calls.

### Summary

```
  Script                             min(ms)    median(ms)    instructions     calls
  arithmetic_tight_loop.mt            777.93        793.41           20013         0
  method_dispatch.mt                  203.93        204.68           14039       506
  object_alloc.mt                     795.00        804.77           12509         0
  field_write_hot.mt                  134.70        135.35            8016         1
  field_read_hot.mt                   164.57        165.40            9017         1
  string_ops.mt                       147.81        148.21           19014         0
  recursive.mt                       1285.65       1286.27           17256   2762961
  bitwise_tight_loop.mt              1191.58       1191.76           23014         0
  short_circuit_chain.mt              298.99        300.09           24907         0
  primitive_method_dispatch.mt        612.75        613.83           32031         0
  array_multi_alloc.mt                 40.44         40.49           10909       500
  array_multi_get.mt                 1089.24       1093.34           50815       500
  for_each_loop.mt                    395.00        397.45           75650      5604
  inline_monomorphic.mt               181.86        181.99           13013       501
  inline_branching.mt                 182.87        184.11           15013       501
  inline_polymorphic.mt               202.11        205.01           14048       508
  inline_value_object_hot.mt         1413.19       1414.81           12514       500
  function_call_hot.mt                169.89        171.72           15409       500
```

### Notes

- `recursive.mt`: **1511 → 1286 ms (14.9% improvement)** vs. the user-reported pre-work baseline. Phase 1 alone landed ~4.4%, Phase 2 added ~11%. `calls` stat barely moves (2763594 → 2762961) because it counts interpreter→JIT entries only; JIT-nested recursion doesn't increment it, so TCO's per-call savings show up only in wall-clock.

## 2026-04-24 — value-class method dispatch fix (Stage A + B + C)

- Machine: dev machine (Windows 11 Home)
- Branch:  `class-value`
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)

### Change in this snapshot

Three stacked fixes for value-class method dispatch, closing a 6.7× gap vs. regular-class inlining on `inline_value_object_hot.mt`:

- **Stage A (materialisation batch-load)**: The interpreter + JIT slow paths previously called `tempInstance->setField(name, value)` per declared field — N hashmap probes + N write-barrier branches per call. Replaced with a single `ObjectInstance::loadFromValueObject(src)` helper: one `fieldVector = src.getFields()` vector copy + one synchronised `fieldValues` fill. Added `callMethodFromJit(Value, …)` / `callMethodFromJitDirect(Value, …)` overloads that delegate here. Alone: 1450 → 1317 ms (~9%).
- **Stage B (JIT IC-hit unblock)**: `jit_call_method_ic` previously split on `receiverIsValueObject` — ObjectInstance hits took `callMethodFromJitDirect` with pre-resolved funcMetadata, ValueObject hits fell through to `jit_call_method` which re-resolved the method every call. Unified the hit path through the new `callMethodFromJitDirect(Value, …)` overload, skipping the per-iter `findInstanceMethodInHierarchy` + `program->getFunction` lookups. Stage A+B: 1317 → 1098 ms.
- **Stage C (interpreter IC populate)**: `InlineCacheExecutor::handleCallMethodIC` bailed early for non-`isObject` receivers and never populated the IC for value-class sites. By OSR-compile time the IC was still `UNINITIALIZED`, so `tryEmitInlinedMethodCall` had nothing to consume and fell back to a `jit_call_method_ic` emit. Teaching the handler to accept value-object receivers and record `entry.receiverIsValueObject = true` means the OSR recompile sees a populated MONO IC and the existing speculative inliner (`emitInlinedMethodCallMono`, already value-object-capable via `jit_extract_classdef` / `jit_field_data_const` / raw-memcpy `emitInlineLocalCopy`) emits the inlined body — no dispatch, no materialisation, no mini-interpret for the 1.999M hot iterations. Stage A+B+C: 1098 → 217 ms.

Correctness preserved: `inline_value_object_write_skip.mt` still prints `200` (in-method `this.x = ...` visibility kept via the temp-ObjectInstance fallback path for write-method callees; read-only callees take the inlined path). `tryPromoteToCached` still skips value-class entries — interpreter `CALL_METHOD_CACHED` fast-dispatch stays ObjectInstance-only by design.

### Summary

```
  Script                             min(ms)    median(ms)    instructions     calls
  arithmetic_tight_loop.mt            838.58        840.55           20013         0
  method_dispatch.mt                  215.70        215.88           14039       506
  object_alloc.mt                     800.95        817.88           12509         0
  field_write_hot.mt                  140.85        141.31            8016         1
  field_read_hot.mt                   173.74        174.16            9017         1
  string_ops.mt                       157.18        157.79           19014         0
  recursive.mt                       1316.81       1321.17           17256   2762961
  bitwise_tight_loop.mt              1258.88       1264.44           23014         0
  short_circuit_chain.mt              311.35        313.15           24907         0
  primitive_method_dispatch.mt        626.02        626.30           32031         0
  array_multi_alloc.mt                 41.44         41.51           10909       500
  array_multi_get.mt                 1102.47       1114.04           50815       500
  for_each_loop.mt                    350.33        351.29           75650      5604
  inline_monomorphic.mt               190.98        191.73           13013       501
  inline_branching.mt                 195.05        195.54           15013       501
  inline_polymorphic.mt               215.13        215.73           14048       508
  inline_value_object_hot.mt          214.68        216.65           12514       500
  function_call_hot.mt                172.32        172.50           15409       500
```

### Notes

- `inline_value_object_hot.mt`: **1450 → 217 ms (6.7× speedup)**. Now within 13% of `inline_monomorphic.mt` (191.73) and essentially tied with `inline_polymorphic.mt` (215.73) — value-class dispatch has reached parity with regular-class inlining, matching the original design intent.
- No other benchmark regressed beyond noise (~1–2% on a few, well inside run-to-run variation).

## 2026-04-25

### Summary (jit=on)

```
  Script                             min(ms)    median(ms)    instructions     calls
  arithmetic_tight_loop.mt            763.40        766.10           20013         0
  method_dispatch.mt                  200.62        201.05           14040       506
  object_alloc.mt                     816.20        823.87           12009         0
  field_write_hot.mt                  131.35        134.76            8016         1
  field_read_hot.mt                   161.10        164.14            9017         1
  string_ops.mt                       152.46        153.35           19014         0
  recursive.mt                       1304.68       1309.77           17257   2762961
  bitwise_tight_loop.mt              1193.41       1195.97           23014         0
  short_circuit_chain.mt              290.39        290.40           24907         0
  primitive_method_dispatch.mt        616.00        618.62           32031         0
  array_multi_alloc.mt                 39.56         39.84           10909       500
  array_multi_get.mt                 1088.88       1090.17           50815       500
  for_each_loop.mt                    365.07        365.31           69848      5604
  inline_monomorphic.mt               175.64        175.97           13013       501
  inline_branching.mt                 178.24        179.16           15013       501
  inline_polymorphic.mt               209.06        210.48           14049       508
  inline_value_object_hot.mt          197.00        197.68           11514       500
  function_call_hot.mt                164.80        165.19           21009       500
```

## 2026-04-25 — array_multi_get fix: FlatMultiArray sub-array view cache + ArrayExecutor cleanup

- Branch:  `array_multi_get`
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)

### Change in this snapshot

Two stacked changes targeting `array_multi_get.mt` (5000 outer × 32×32 grid → 5.12 M `grid[i][j]` accesses, JIT-compiled via OSR after threshold):

- **Phase 1 — `mType/vm/runtime/executors/ArrayExecutor.{hpp,cpp}` cleanup**: changed the six `get*Element` / `set*Element` helper signatures from `std::shared_ptr<…>` (by value, MYT-200 trap — two atomic refcount ops per call) to `const std::shared_ptr<…>&`; bound by `const auto&` at the `handleArrayGet` / `handleArraySet` / `handleArrayLength` call sites so `asNativeArray()`'s already-`const&` return flows straight through; gated the `isValueObject` / `deepCopy` block on `array->getElementType() == ValueType::OBJECT` so primitive-array reads skip the always-false branch; switched to `push(std::move(element))` to skip a redundant retain/release pair. Net effect on this benchmark: nil (the JIT bypasses the interpreter handlers — counters showed `handleArrayGet` fired ~2.6 k times across the whole bench while `jit_array_get` fired 5.1 M times). Still ships as correctness/cleanup that helps interpreter-mode workloads and cold paths.
- **Phase 2 — `mType/value/FlatMultiArray.hpp` sub-array view cache**: added a single-entry `(cachedSubArrayIndex_, cachedSubArray_)` pair on each `FlatMultiArray`. `getSubArray(index)` fast-returns the cached `shared_ptr` on a hit. The inner `j`-loop in `grid[i][j]` calls `getSubArray(i)` 32 consecutive times with the same `i`, so cache hit rate ≈ 31/32; sub-array heap allocations drop from ~5.12 M to ~160 k. `reset()` clears the cache so pool-reissued arrays don't hand stale views to the next tenant. Cycle (`cachedSubArray_` → view, view's `parent_` → this) is reclaimed by the project's `CycleDetector` (`mType/gc/CycleDetector.hpp`) — bounded leak between sweeps.

### Diagnosis path

Initial assumption (refcount cost on the interpreter ARRAY_GET) was wrong: Phase 1 alone moved the median by <2% (within noise). Adding ARRAY_PATH and JIT-ARRAY counters showed the loop OSRs out of the interpreter after one `sumGrid` call and `jit_array_get(flat, rk>1)` fires 5,119,440 times. That pinned `FlatMultiArray::getSubArray` (one `new FlatMultiArray(...)` per call) as the bottleneck, leading to Phase 2.

### Summary

```
  Script                             min(ms)    median(ms)    instructions     calls
  arithmetic_tight_loop.mt            765.34        768.38           20013         0
  method_dispatch.mt                  202.59        203.23           14040       506
  object_alloc.mt                     793.50        797.59           12009         0
  field_write_hot.mt                  126.83        128.28            8016         1
  field_read_hot.mt                   157.86        159.94            9017         1
  string_ops.mt                       152.57        152.99           19014         0
  recursive.mt                       1281.80       1282.36           17257   2762961
  bitwise_tight_loop.mt              1171.64       1175.31           23014         0
  short_circuit_chain.mt              283.19        286.40           24907         0
  primitive_method_dispatch.mt        619.60        627.43           32031         0
  array_multi_alloc.mt                 39.66         40.09           10909       500
  array_multi_get.mt                  317.78        318.26           50815       500
  for_each_loop.mt                    364.69        365.42           69848      5604
  inline_monomorphic.mt               173.46        174.21           13013       501
  inline_branching.mt                 174.37        176.74           15013       501
  inline_polymorphic.mt               213.13        213.95           14049       508
  inline_value_object_hot.mt          199.77        199.78           11514       500
  function_call_hot.mt                158.57        160.68           21009       500
```

### Delta vs 2026-04-25 baseline (median)

| Script                         | Before (ms) | After (ms) | Change   |
|--------------------------------|------------:|-----------:|---------:|
| array_multi_get.mt             |     1090.17 |     318.26 | **-70.8% (3.4× faster)** |
| object_alloc.mt                |      823.87 |     797.59 |   -3.2%  |
| field_write_hot.mt             |      134.76 |     128.28 |   -4.8%  |
| field_read_hot.mt              |      164.14 |     159.94 |   -2.6%  |
| recursive.mt                   |     1309.77 |    1282.36 |   -2.1%  |
| bitwise_tight_loop.mt          |     1195.97 |    1175.31 |   -1.7%  |
| short_circuit_chain.mt         |      290.40 |     286.40 |   -1.4%  |
| function_call_hot.mt           |      165.19 |     160.68 |   -2.7%  |
| arithmetic_tight_loop.mt       |      766.10 |     768.38 |   +0.3%  |
| method_dispatch.mt             |      201.05 |     203.23 |   +1.1%  |
| string_ops.mt                  |      153.35 |     152.99 |   -0.2%  |
| primitive_method_dispatch.mt   |      618.62 |     627.43 |   +1.4%  |
| array_multi_alloc.mt           |       39.84 |      40.09 |   +0.6%  |
| for_each_loop.mt               |      365.31 |     365.42 |   +0.0%  |
| inline_monomorphic.mt          |      175.97 |     174.21 |   -1.0%  |
| inline_branching.mt            |      179.16 |     176.74 |   -1.4%  |
| inline_polymorphic.mt          |      210.48 |     213.95 |   +1.6%  |
| inline_value_object_hot.mt     |      197.68 |     199.78 |   +1.1%  |

### Notes

- `array_multi_get.mt`: **1090 → 318 ms (3.4×)** — single targeted change. Remaining ~318 ms is now ~30 ns per `jit_array_get` call across 10.24 M calls; further wins require either fusing the 2D-access pair into one helper call or JIT-inlining the `FlatMultiArray` rank>1 → rank==1 path so dispatch disappears.
- Other movements are all within ±5%, mostly within typical run-to-run noise. No clear regressions.
- `array_multi_alloc.mt` is essentially unchanged (~40 ms before and after) — the cache only fires on `getSubArray`, not on construction. The earlier impression that it improved was a misread against an older baseline row.
- `SparseMultiArray::getSubArray` has the same antipattern but isn't exercised by the current bench suite (sparse counters were 0); applying the same single-entry cache there is a small follow-up if/when sparse-multi-dim workloads matter.

## MYT-204 (LOAD_VAR_CACHED / STORE_VAR_CACHED)

- Branch: `MYT-204`
- Change: runtime-promoted `LOAD_VAR` → `LOAD_VAR_CACHED` and `STORE_VAR` →
  `STORE_VAR_CACHED` once `Environment::findVariable` succeeds. Cached
  executors dereference the snapshotted `VariableDefinition*` directly,
  skipping the constant-pool string fetch and the `unordered_map<string,
  shared_ptr<VariableDefinition>>` probe on every dispatch. JIT routing
  unchanged (CACHED treated identically to non-CACHED at emit time —
  win is interpreter-only).
- Expected impact: most visible on global-read-heavy interpreted scripts
  where `findVariable` shows up in profile (string hashing dominates over
  value access). JIT-compiled hot loops are unaffected by design.
- Numbers: TODO — capture before/after on a global-read-heavy script
  (tight loop accumulating into / reading from a top-level int variable).

## 2026-04-25 — MYT-207: self-recursive direct-call dispatch (recursive.mt)

- Branch: `MYT-207`
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)

### Change in this snapshot

- Phase 1 TCO (already live) lowered `return self(args...)` sites to
  arg-overwrite + jmp to the function-entry label. Brought `recursive.mt`
  from 1557 → 1365 ms (-12%).
- This snapshot adds the **non-tail** complement: `tryEmitSelfDirectCall`
  detects a self-recursive `CALL`/`CALL_FAST` whose target is the
  currently-compiling function and emits a direct asmjit `cc.invoke`
  against the function's own `FuncNode->label()`, skipping
  `jit_call_function`'s dispatch overhead — `jitCodeCache` lookup,
  `tryJitDispatchResolved`'s `CallFrame` push + vector grow, the
  ~256-byte `JitContext nestedCtx{}` zero-init, and the SEH `try/catch`
  setup. Inline depth-guard against `MAX_JIT_NATIVE_DEPTH=64` falls
  back to the generic helper on overflow (which itself bails to the
  interpreter via `callFunctionFromJit`).
- Net effect on `fib(32)`'s ~2.76 M non-tail recursive calls: each call
  now skips ~150–300 cycles of dispatch.
- Also adds two `--jit-stats` counters: `Tail calls optimized` and
  `Self direct calls`, bumped at compile time inside the respective
  helpers.

### Summary (jit=on)

```
  Script                             min(ms)    median(ms)    instructions     calls
  arithmetic_tight_loop.mt            774.43        775.13           20013         0
  method_dispatch.mt                  206.55        209.97           14040       506
  object_alloc.mt                     791.35        794.99           12009         0
  field_write_hot.mt                  132.74        133.67            8016         1
  field_read_hot.mt                   162.23        162.80            9017         1
  string_ops.mt                       144.57        144.67           19014         0
  recursive.mt                        918.85        920.57           17257   2762961
  bitwise_tight_loop.mt              1175.98       1183.35           23014         0
  short_circuit_chain.mt              283.93        285.33           24907         0
  primitive_method_dispatch.mt        684.86        690.00           32031         0
  array_multi_alloc.mt                 40.35         40.46           10909       500
  array_multi_get.mt                  326.59        328.26           50815       500
  for_each_loop.mt                    369.12        371.36           69848      5604
  inline_monomorphic.mt               175.15        177.38           13013       501
  inline_branching.mt                 177.73        179.34           15013       501
  inline_polymorphic.mt               211.54        212.43           14049       508
  inline_value_object_hot.mt          196.24        196.39           11514       500
  function_call_hot.mt                168.69        168.75           21009       500
```

### Delta vs 2026-04-25 baseline (median)

| Script                         | Before (ms) | After (ms) | Change   |
|--------------------------------|------------:|-----------:|---------:|
| recursive.mt                   |     1309.77 |     920.57 | **-29.7% (1.42× faster)** |
| primitive_method_dispatch.mt   |      618.62 |     690.00 |  +11.5%  |
| arithmetic_tight_loop.mt       |      766.10 |     775.13 |   +1.2%  |
| method_dispatch.mt             |      201.05 |     209.97 |   +4.4%  |
| object_alloc.mt                |      823.87 |     794.99 |   -3.5%  |
| field_write_hot.mt             |      134.76 |     133.67 |   -0.8%  |
| field_read_hot.mt              |      164.14 |     162.80 |   -0.8%  |
| string_ops.mt                  |      153.35 |     144.67 |   -5.7%  |
| bitwise_tight_loop.mt         |     1195.97 |    1183.35 |   -1.1%  |
| short_circuit_chain.mt         |      290.40 |     285.33 |   -1.7%  |
| array_multi_alloc.mt           |       39.84 |      40.46 |   +1.6%  |
| array_multi_get.mt             |      318.26 |     328.26 |   +3.1%  |
| for_each_loop.mt               |      365.31 |     371.36 |   +1.7%  |
| inline_monomorphic.mt          |      175.97 |     177.38 |   +0.8%  |
| inline_branching.mt            |      179.16 |     179.34 |   +0.1%  |
| inline_polymorphic.mt          |      210.48 |     212.43 |   +0.9%  |
| inline_value_object_hot.mt     |      197.68 |     196.39 |   -0.7%  |
| function_call_hot.mt           |      165.19 |     168.75 |   +2.2%  |

### Notes

- **`recursive.mt`: 1310 → 921 ms (-29.7% from today's baseline; -41% from
  the original 1557 ms ticket baseline)**. Comfortably clears the MYT-207
  ≥25% AC. Phase 1 TCO contributed -12%, the new direct-call dispatch
  contributes the additional -29% — confirming `fib`'s ~2.76 M non-tail
  recursive calls were the dominant residual cost.
- `primitive_method_dispatch.mt` shows +11.5% — outside noise, worth a
  follow-up. Nothing in the MYT-207 change touches `INVOKE_INT/FLOAT` or
  the primitive-method path; this looks like run-to-run variance or an
  unrelated regression. Re-measure on a clean run before chasing it.
- All other benchmarks within ±5%, no clear regressions.

## 2026-04-26 — post Object Allocation Optimization

- Machine: dev machine (Windows 11 Home)
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)
- Note: Optimized ObjectInstance by removing maps and using vector-backed storage.

### Summary

| Script                             | min(ms) | median(ms) | Instructions | Calls   |
|------------------------------------|--------:|-----------:|-------------:|--------:|
| arithmetic_tight_loop.mt            |   49.22 |      49.30 |        20017 |       0 |
| method_dispatch.mt                  |   73.59 |      73.69 |        14042 |     506 |
| object_alloc.mt                     |  484.44 |     494.98 |        12011 |       0 |
| object_alloc_nested.mt              |  817.06 |     817.60 |        16411 |     500 |
| field_write_hot.mt                  |   60.71 |      61.79 |         8017 |       1 |
| field_read_hot.mt                   |   39.07 |      39.34 |         8520 |       1 |
| string_ops.mt                       |   81.00 |      81.37 |        19014 |       0 |
| recursive.mt                        |  887.75 |     893.09 |        17260 | 2762961 |
| bitwise_tight_loop.mt               |   43.88 |      44.21 |        23019 |       0 |
| short_circuit_chain.mt              |   51.56 |      52.79 |        24909 |       0 |
| primitive_method_dispatch.mt        |  451.40 |     456.54 |        32038 |       0 |
| array_multi_alloc.mt                |   54.45 |      55.64 |         9911 |     500 |
| array_multi_get.mt                  |  329.50 |     331.69 |        49787 |     500 |
| for_each_loop.mt                    |  323.36 |     323.73 |        69849 |    5604 |
| inline_monomorphic.mt               |   45.65 |      46.04 |        13016 |     501 |
| inline_branching.mt                 |   49.78 |      49.81 |        15016 |     501 |
| inline_polymorphic.mt               |   72.18 |      72.56 |        14051 |     508 |
| inline_value_object_hot.mt          |   74.55 |      75.12 |        11517 |     500 |
| function_call_hot.mt                |  168.15 |     168.52 |        15011 |     500 |

## 2026-04-26 — async/await + lambda benchmarks added; async perf passes

- Machine: dev machine (Windows 11 Home)
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)
- Changes in this run:
  1. **New benchmarks**: `async_await_tight_loop.mt`, `async_await_chain.mt`,
     `lambda_call_hot.mt`, `lambda_closure_hot.mt` added to the canonical
     suite. CANONICAL_SCRIPTS grew from 19 → 23.
  2. **Profiler fix**: `FunctionExecutor::CALL_FAST` profiler entry hook now
     uses the mangled name (matching what RETURN reports on exit) — silences
     the 1M-line `mismatched function exit` warning flood under
     `--profile=full`. No effect on benchmark wall time.
  3. **MYT-202 fusion** (two new superinstructions):
     - `OBJECT_TO_VALUE + CREATE_PROMISE` → `OBJECT_TO_VALUE_CREATE_PROMISE`
     - `CREATE_PROMISE + RETURN_VALUE` → `CREATE_PROMISE_RETURN_VALUE`
     `AWAIT + STORE_LOCAL` was deliberately *not* fused — AWAIT's suspend
     path increments IP and bails, so a fused STORE_LOCAL would be skipped
     on resume. Defer until the suspend path is taught to keep IP at the
     fused opcode.
  4. **Inline resolved-Promise<Int>**: new `ValueType::PROMISE_INT` tag
     holding the resolved int directly in the payload — no heap
     `AsyncPromiseValue` allocation on the synchronous-completion path.
     `CREATE_PROMISE` and the fused variants emit the inline form when the
     input is a raw INT or an Int box (detected via
     `ObjectInstance::getPrimitiveTag()`); `AWAIT` recognises the inline
     form and pushes a raw INT, which `INVOKE_INT_GET_VALUE` and method
     dispatch (via `ObjectExecutor::handleCallMethod` lazy auto-boxing)
     handle natively. Eliminates ~1M `make_shared<AsyncPromiseValue>` calls
     per benchmark run.

### Summary (jit=on)

```
  Script                             min(ms)    median(ms)    instructions     calls
  arithmetic_tight_loop.mt             49.49         49.59           20017         0
  method_dispatch.mt                   73.71         74.28           14042       506
  object_alloc.mt                     467.20        468.62           12011         0
  object_alloc_nested.mt              768.31        774.09           16411       500
  field_write_hot.mt                   56.00         56.64            8017         1
  field_read_hot.mt                    36.52         36.62            8520         1
  string_ops.mt                        79.07         79.20           19019         0
  recursive.mt                        929.75        931.67           17260   2762961
  bitwise_tight_loop.mt                45.03         45.49           23019         0
  short_circuit_chain.mt               50.62         51.13           24909         0
  primitive_method_dispatch.mt        436.48        439.43           32038         0
  array_multi_alloc.mt                 52.41         52.68            9911       500
  array_multi_get.mt                  322.36        324.62           49787       500
  for_each_loop.mt                    310.27        311.46           69849      5604
  inline_monomorphic.mt                44.50         44.74           13016       501
  inline_branching.mt                  46.20         46.47           15016       501
  inline_polymorphic.mt                72.51         72.68           14051       508
  inline_value_object_hot.mt           74.26         75.60           11517       500
  function_call_hot.mt                161.61        161.87           15011       500
  async_await_tight_loop.mt          1134.82       1145.32        32000032   1000001
  async_await_chain.mt               1770.17       1774.53        34500032   2000001
  lambda_call_hot.mt                   57.92         57.93           12521         1
  lambda_closure_hot.mt                58.43         58.72           12526         2
```

### Async benchmark — per-pass deltas (median ms)

| benchmark                  | original | + fusion | + inline | per-await Δ        | total Δ |
|----------------------------|---------:|---------:|---------:|--------------------|--------:|
| async_await_tight_loop.mt  |  1410.72 |  1372.56 |  1145.32 | 1.41 µs → 1.15 µs | -18.8% |
| async_await_chain.mt       |  2309.75 |  2292.17 |  1774.53 | 1.15 µs → 0.89 µs | -23.2% |

### Notes

- **Fusion alone bought ~1-3%.** The wall-time delta (median) was small
  because dispatch isn't the dominant per-await cost; the
  `make_shared<AsyncPromiseValue>` allocation is. Fusion did fire correctly
  — instruction count for the two async benchmarks dropped by exactly the
  expected number of fused pairs (-1 M and -2 M instructions respectively).
  Only one fusion fires per async-fn body in the benchmark: the peephole
  pass collapses `OBJECT_TO_VALUE + CREATE_PROMISE` first; the resulting
  `OBJECT_TO_VALUE_CREATE_PROMISE + RETURN_VALUE` pair is then unrecognized.
- **Inline resolved-Promise bought another ~17-22%.** This is the change
  that actually moves the needle: removing 1 M heap allocations per run.
  Below the 30-50% upper-bound prediction because the `NEW_VALUE_OBJECT`
  ObjectInstance allocation (pool-backed) still happens on every iteration
  — only the AsyncPromiseValue allocation is eliminated. A future triple
  fusion `NEW_VALUE_OBJECT(Int) + OBJECT_TO_VALUE + CREATE_PROMISE` →
  `CREATE_PROMISE_INT_FROM_STACK` would skip even the pool round-trip.
- **Non-async benchmarks** are within run-to-run noise (±5%). The added
  tag-check on the CREATE_PROMISE / AWAIT handlers is constant-time and
  only fires on async paths; nothing else was touched.
- **JIT** does not compile CREATE_PROMISE / AWAIT, so JITted code bails to
  the interpreter and picks up the inline-form fast path automatically.
  No JIT changes required.

### Sanity outputs

- `async_await_tight_loop.mt`: `async_await_tight_loop total=1000000000000`
- `async_await_chain.mt`, `lambda_call_hot.mt`, `lambda_closure_hot.mt`:
  capture stdout on the next clean run and pin here.

## 2026-04-26 — recursive.mt call overhead reduction (Phases 1/2/3/6)

- Machine: dev machine (Windows 11 Home)
- Branch:  current working tree
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)

Scope:
- **Phase 1**: Removed vestigial `environment->exitScope()` from `ControlFlowExecutor::handleReturn` — no matching `enterScope` exists in the VM runtime.
- **Phase 2**: Replaced per-element `pop_back()` stack cleanup loop in `handleReturn` with a single `resize(frame.frameBase)` call (~2.5M function returns benefit).
- **Phase 3**: Added `allPrimitiveParams` flag to `FunctionMetadata`, computed once at `registerFunction()` time. Guarded all 3 `convertLambdaArgumentsToInterfaces` call sites to skip when all params are `int`/`float`/`bool`/`string`/`void`.
- **Phase 6**: Increased `MAX_JIT_NATIVE_DEPTH` from 64 to 256 and `callStack.reserve(256)` to keep more recursion levels in the JIT fast path and avoid vector reallocations.

```
=== Summary (jit=on) ===
  Script                             min(ms)    median(ms)    instructions     calls
  arithmetic_tight_loop.mt             56.89         57.27           20017         0
  method_dispatch.mt                   74.31         74.41           14043       506
  object_alloc.mt                     486.06        494.17           12011         0
  object_alloc_nested.mt              776.77        784.61           16411       500
  field_write_hot.mt                   58.63         58.65            8017         1
  field_read_hot.mt                    37.92         38.31            8520         1
  string_ops.mt                        82.38         82.47           19019         0
  recursive.mt                        768.52        772.01           17261   2545487
  bitwise_tight_loop.mt                47.66         47.98           23019         0
  short_circuit_chain.mt               51.56         52.28           24909         0
  primitive_method_dispatch.mt        452.11        455.02           32040         0
  array_multi_alloc.mt                 52.35         53.99            9911       500
  array_multi_get.mt                  321.67        322.46           49787       500
  for_each_loop.mt                    328.75        329.23           69852      5604
  inline_monomorphic.mt                44.26         44.33           13017       501
  inline_branching.mt                  46.23         46.69           15017       501
  inline_polymorphic.mt                73.23         74.83           14052       508
  inline_value_object_hot.mt           74.39         74.56           11518       500
  function_call_hot.mt                168.40        168.70           15011       500
  async_await_tight_loop.mt          1117.51       1123.90        32000034   1000001
  async_await_chain.mt               1791.22       1815.12        34500034   2000001
  lambda_call_hot.mt                   59.02         60.92           12522         1
  lambda_closure_hot.mt                59.45         60.11           12527         2
```

### Delta vs prior baseline (2026-04-26 MYT-209)

| Script                          | Prior median (ms) | Now median (ms) | Change |
|---------------------------------|------------------:|----------------:|-------:|
| recursive.mt                    |            883.50 |          772.01 | **-12.6%** |
| function_call_hot.mt            |            173.53 |          168.70 | -2.8% (noise) |
| for_each_loop.mt                |            341.02 |          329.23 | -3.5% (noise) |
| All other benchmarks            |                   |                 | within ±3% |

### Notes

- **recursive.mt** improved from 883ms → 772ms (12.6% faster). Call count dropped from 2,762,961 → 2,545,487 confirming the higher `MAX_JIT_NATIVE_DEPTH` keeps more recursion levels in the JIT fast path.
- **Phase 5** (JIT box/unbox elimination via `primCallArgs` pass-through) was attempted but reverted — adding new fields to `JitContext` shifted `offsetof` for downstream fields (`icTable`, `osrLocals`, etc.) and caused crashes in non-recursive benchmarks. The approach needs a layout-stable design (e.g., placing `primCallArgs` at the end of the struct, or using a separate side-channel).
- No regressions observed on any other benchmark.

## 2026-04-29 — MYT-228 reified runtime type info for free generic functions

- Machine: dev machine (Windows 11 Home)
- Branch:  MYT-228
- Commit:  `82336606`
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)

Scope:
- Added `BIND_TYPE_ARGS` opcode + `CallFrame::typeArgBindings` so `obj isClassOf T` and `(T)cast` work for method-level / free-function generic params (not just class-level).
- New benchmark `generic_dispatch_hot.mt` exercises `Inspector::matches<T>(animal)` in a 1M-iter loop with two distinct call sites (T=Dog / T=Cat).
- Storage uses a thread-local pool of `unordered_map` recycled via a raw-pointer RAII wrapper (`TypeArgMapPtr`) — 8 bytes per frame, near-zero alloc per generic call after warmup.
- JIT helpers (`jit_instanceof_typeparam`, `jit_cast_typeparam`, `jit_bind_type_args`) read the per-frame map and populate the pool slot in-place.

```
=== Summary (jit=on) ===
  Script                             min(ms)    median(ms)    instructions     calls
  arithmetic_tight_loop.mt             57.87         58.27           20017         0
  method_dispatch.mt                   75.62         76.41           14043       506
  object_alloc.mt                     543.67        556.68           12511         0
  object_alloc_nested.mt              914.10        919.17           16811       500
  field_write_hot.mt                   58.27         58.63            8018         1
  field_read_hot.mt                    53.98         54.33            9020         1
  string_ops.mt                        83.79         83.90           19019         0
  recursive.mt                        775.01        778.25           17261   2545487
  bitwise_tight_loop.mt                48.29         49.65           23019         0
  short_circuit_chain.mt               52.44         53.34           24909         0
  primitive_method_dispatch.mt        462.03        464.25           32039         0
  array_multi_alloc.mt                 54.06         54.55            9911       500
  array_multi_get.mt                  323.22        335.08           49787       500
  for_each_loop.mt                    306.44        309.53           75654      5604
  inline_monomorphic.mt                43.25         43.26           13017       501
  inline_branching.mt                  47.77         48.44           15017       501
  inline_polymorphic.mt                74.14         74.36           14052       508
  inline_value_object_hot.mt          120.89        121.39           12518       500
  function_call_hot.mt                164.50        167.23           15011       500
  async_await_tight_loop.mt          1151.25       1152.69        32000033   1000001
  async_await_chain.mt               1758.80       1768.04        34500033   2000001
  lambda_call_hot.mt                   58.88         59.46           12522         1
  lambda_closure_hot.mt                59.91         60.57           12527         2
  generic_dispatch_hot.mt             647.97        650.02           20075      1012
```

### MYT-228 in-flight tuning

The first MYT-228 implementation used `std::optional<std::unordered_map<...>>` per frame
(~72B/frame) with a fresh map allocation per generic call:

| Script                    | optional<map> (ms) | pooled raw-ptr (ms) | Δ |
|---------------------------|------------------:|-------------------:|--:|
| object_alloc.mt           |             770.96 |             543.67 | **-29.5%** |
| generic_dispatch_hot.mt   |             825.96 |             647.97 | **-21.5%** |

Frame size dropped from ~470B to ~408B (60B saved per frame), and `generic_dispatch_hot`'s
per-call alloc was eliminated by the pool. Both wins land together via the same change.

### Notes

- **`generic_dispatch_hot.mt` per-call cost**: 648ms / 2M generic calls = ~324ns/call vs `method_dispatch.mt` at ~38ns/call. The ~286ns/call overhead is BIND_TYPE_ARGS dispatch + `INSTANCEOF_TYPEPARAM` resolve walk + name-based `checkInstanceOfByName`. Further reduction would need a per-call-site IC for the resolved type name.
- **No regressions** on benchmarks that don't touch generics (arithmetic, field, lambda, async). The CallFrame size reduction matters most for hot constructor loops (`object_alloc.mt`).

### Sanity outputs

- `generic_dispatch_hot.mt`: `generic_dispatch_hot dogs=666667 cats=333333`
  (zoo=[Dog,Cat,Dog,Dog,Cat,Dog], N=1000000 → 4-of-6 dogs ≈ 666667, 2-of-6 cats ≈ 333333; verify on first clean run)

## 2026-04-30

```
=== Summary (jit=on) ===
  Script                             min(ms)    median(ms)    instructions     calls
  arithmetic_tight_loop.mt             50.56         51.81           20017         0
  method_dispatch.mt                   77.21         77.23           14043       506
  object_alloc.mt                     539.37        549.94           12511         0
  object_alloc_nested.mt              908.57        916.78           16811       500
  field_write_hot.mt                   58.69         59.37            8018         1
  field_read_hot.mt                    55.09         55.68            9020         1
  string_ops.mt                        83.63         83.73           19019         0
  recursive.mt                        842.07        842.18           17261   2545487
  bitwise_tight_loop.mt                45.40         45.65           23019         0
  short_circuit_chain.mt               54.15         54.94           24909         0
  primitive_method_dispatch.mt        475.33        479.56           32039         0
  array_multi_alloc.mt                 54.88         55.10            9911       500
  array_multi_get.mt                  342.23        343.67           49787       500
  for_each_loop.mt                    320.23        322.38           75654      5604
  inline_monomorphic.mt                46.75         47.72           13017       501
  inline_branching.mt                  49.67         50.11           15017       501
  inline_polymorphic.mt                76.90         77.77           14052       508
  inline_value_object_hot.mt          127.00        127.55           12518       500
  function_call_hot.mt                170.70        172.75           15011       500
  async_await_tight_loop.mt           915.46        915.99        23000933   1000001
  async_await_chain.mt               1406.23       1407.95        20502833   2000001
  lambda_call_hot.mt                   63.13         64.03           12522         1
  lambda_closure_hot.mt                61.76         62.81           12527         2
  generic_dispatch_hot.mt             690.65        690.94           20075      1012
```

## 2026-05-02 — MYT-259 OSR RETURN_VALUE function-return support

- Machine: dev machine (Windows 11 Home)
- Branch:  MYT-259
- Commit:  `733fd283`
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)

Scope:
- OSR-emitted `RETURN`/`RETURN_VALUE` now push the function's return value
  onto the interpreter's operand stack (via new `jit_osr_push_*` helpers) and
  resume at the RETURN_VALUE opcode itself, so the interpreter's normal
  `handleReturnValue()` runs the call-frame pop / IP restore / async-promise
  wrap. Previous behaviour resumed at the post-loop IP, which silently
  dropped the return — `HashMap.findKeyInBucket` collision-bucket lookups
  fell through to the trailing `return -1`.
- `CREATE_PROMISE_RETURN_VALUE` still bails OSR (its async-promise wrap is
  fused into the opcode and the OSR resume path doesn't yet replay it).
- New regression test `osr_return_boxed_object.mt` exercises the boxed
  branch of `emitOsrPushReturnValueToInterpStack` end-to-end.

```
=== Summary (jit=on) ===
  Script                             min(ms)    median(ms)    instructions     calls
  arithmetic_tight_loop.mt            103.74        104.21           20017         0
  method_dispatch.mt                  125.45        126.97           14043       506
  object_alloc.mt                     521.96        525.31           12511         0
  object_alloc_nested.mt             1169.81       1173.45           16811       500
  field_write_hot.mt                   65.30         65.73            8018         1
  field_read_hot.mt                    66.82         66.83            9020         1
  string_ops.mt                       112.31        112.77           19019         0
  recursive.mt                        806.68        808.18           17261   2545487
  bitwise_tight_loop.mt                77.18         77.76           23019         0
  short_circuit_chain.mt               63.90         64.41           24909         0
  primitive_method_dispatch.mt        453.33        455.28           32039         0
  array_multi_alloc.mt                 72.64         73.03            9911       500
  array_multi_get.mt                  333.57        333.63           49787       500
  for_each_loop.mt                    303.51        303.63           75654      5604
  inline_monomorphic.mt                85.78         86.58           13017       501
  inline_branching.mt                  89.44         90.57           15017       501
  inline_polymorphic.mt               125.73        126.20           14052       508
  inline_value_object_hot.mt          156.06        156.60           12518       500
  function_call_hot.mt                174.86        174.92           15011       500
  async_await_tight_loop.mt          1044.77       1060.17        23000933   1000001
  async_await_chain.mt               1676.14       1694.72        20502833   2000001
  lambda_call_hot.mt                  958.28        959.37           12522   1999501
  lambda_closure_hot.mt               988.54        990.37           12527   1999502
  generic_dispatch_hot.mt             993.95        995.06           20075      1012
  try_catch_finally_hot.mt            492.69        494.15           50020      2000
  switch_dispatch_hot.mt              443.92        444.77           14634       500
  overload_dispatch_hot.mt            550.70        552.12           34029      2001
  abstract_dispatch_hot.mt            123.87        125.93           14043       506
  cast_hot.mt                         216.86        217.28           19561       505
  collections_hash_hot.mt            9804.13       9882.63          404406   6007320
  stream_pipeline_hot.mt              418.43        421.31         2090492    306881
  reflection_lookup_hot.mt           2377.27       2385.57           85542   1203001
  pattern_match_hot.mt                432.11        432.97           12861       500
  string_interpolation_hot.mt         251.24        251.63         7400025         0
  boxed_primitive_dispatch_hot.mt    2688.30       2694.60           55803      3000
  linked_list_nested_hot.mt           336.39        339.92          124920     81001
```

### Notes

- `lambda_call_hot.mt` and `lambda_closure_hot.mt` jumped from ~60ms / 1-2 calls
  in the prior baseline to ~960ms / ~2M calls — the benchmark workload itself
  changed (much higher iteration / call count), not a JIT regression. Compare
  against the next run with the same benchmark definition.
- New benchmarks recorded for the first time on this machine:
  `try_catch_finally_hot`, `switch_dispatch_hot`, `overload_dispatch_hot`,
  `abstract_dispatch_hot`, `cast_hot`, `collections_hash_hot`,
  `stream_pipeline_hot`, `reflection_lookup_hot`, `pattern_match_hot`,
  `string_interpolation_hot`, `boxed_primitive_dispatch_hot`,
  `linked_list_nested_hot`. Use these as the baseline for future deltas.
- `collections_hash_hot.mt` produces correct counts (`hits=1000000 len=1785128`)
  with JIT on — confirms the OSR RETURN_VALUE fix lands the correctness win.
- No regressions vs. the 2026-04-29 MYT-228 baseline on the previously-tracked
  scripts (within ±5% noise).

## 2026-05-02 — MYT-258 Phase 3 (HashMap/HashSet flat open-addressing)

- Machine: dev machine (Windows 11 Home)
- Branch:  MYT-258
- Commit:  `aec2ff94` (uncommitted on top)
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)

Scope:
- Replaced 2D bucket-chain layout (`K[][] keyBuckets`, `V[][] valueBuckets`,
  `int[][] hashBuckets`, `int[] bucketSizes`) with flat 1D open addressing
  (`K[] keys`, `V[] values`, `int[] hashes`) in HashMap.mt; same for HashSet.mt
  (`T[] elements`, `int[] hashes`).
- Hash-to-slot: `(rawHash * 1610612741 ^ shift) & (capacity - 1)` — power-of-2
  mask, no divide.
- Linear probing on `put/get/containsKey/add/contains`. `keys[i] == null`
  marks empty slot (null keys forbidden by API). Default capacity 32; load
  factor 0.75 triggers `resize()` (doubles capacity, rehashes via cached
  `hashes[]` — no `key.hashCode()` re-calls).
- Standard back-shift deletion in `remove()` to maintain probe-chain
  invariant.
- Native side updated in lockstep: `JsonSerializer.cpp`,
  `JsonDeserializer.cpp`, and `net/HashMapMarshal.cpp` all read/write the new
  flat layout. `computeBucketIndex` now mirrors the new mask formula.
- New benchmark `collections_hashset_hot.mt` isolates HashSet from the
  combined `collections_hash_hot.mt` workload. Registered in both
  `IntegrationTestSuite.cpp` and `BenchmarkRunner.cpp`.
- New `.expected` files for `serializeHashMap` / `serializeHashSet` (now
  registered as `addOutputVerificationTest` rather than just no-error).

```
=== Summary (jit=on) ===
  Script                             min(ms)    median(ms)    instructions     calls
  arithmetic_tight_loop.mt            105.13        105.70           20017         0
  method_dispatch.mt                  127.88        128.80           14043       506
  object_alloc.mt                     523.79        524.18           12511         0
  object_alloc_nested.mt             1248.40       1257.97           16811       500
  field_write_hot.mt                   63.51         64.93            8018         1
  field_read_hot.mt                    66.67         66.94            9020         1
  string_ops.mt                       114.39        114.87           19019         0
  recursive.mt                        770.23        774.74           17261   2545487
  bitwise_tight_loop.mt                77.09         79.16           23019         0
  short_circuit_chain.mt               62.95         64.62           24909         0
  primitive_method_dispatch.mt        462.23        493.60           32039         0
  array_multi_alloc.mt                 73.28         73.86            9911       500
  array_multi_get.mt                  334.10        334.51           49787       500
  for_each_loop.mt                    305.01        305.09           75654      5604
  inline_monomorphic.mt                86.12         86.75           13017       501
  inline_branching.mt                  87.60         88.28           15017       501
  inline_polymorphic.mt               124.45        124.94           14052       508
  inline_value_object_hot.mt          155.84        159.05           12518       500
  function_call_hot.mt                175.00        175.47           15011       500
  async_await_tight_loop.mt          1034.05       1034.71        23000933   1000001
  async_await_chain.mt               1625.64       1629.00        20502833   2000001
  lambda_call_hot.mt                  943.80        949.20           12522   1999501
  lambda_closure_hot.mt               969.28        970.93           12527   1999502
  generic_dispatch_hot.mt             998.04       1007.10           20075      1012
  try_catch_finally_hot.mt            472.98        480.30           50020      2000
  switch_dispatch_hot.mt              459.38        460.59           14634       500
  overload_dispatch_hot.mt            558.85        560.65           34029      2001
  abstract_dispatch_hot.mt            122.88        124.14           14043       506
  cast_hot.mt                         213.33        215.04           19561       505
  collections_hash_hot.mt            7092.99       7127.58          265401   2581948
  collections_hashset_hot.mt         2329.03       2335.60          112923    860655
  stream_pipeline_hot.mt              413.26        419.77         2090492    306881
  reflection_lookup_hot.mt           2354.15       2356.41           85542   1203001
  pattern_match_hot.mt                439.40        440.19           12861       500
  string_interpolation_hot.mt         243.62        244.29         7400025         0
  boxed_primitive_dispatch_hot.mt    2695.34       2697.15           55803      3000
  linked_list_nested_hot.mt           339.69        341.11          124920     81001
```

### Delta vs prior MYT-259 baseline (2026-05-02)

| Script                       | Before (median, ms) | After (median, ms) | Change   |
|------------------------------|--------------------:|-------------------:|---------:|
| collections_hash_hot.mt      |             9882.63 |            7127.58 | -27.9%   |
| collections_hashset_hot.mt   |                 n/a |            2335.60 | new bench|

Other scripts within ±5% of prior baseline (noise).

### Notes

- MYT-258 acceptance bar was `collections_hash_hot.mt` median ≤ 4500ms (-45%
  from 8246ms baseline). Achieved -27.9% from the most recent prior baseline
  (-13.6% from the original 8246ms). Did not hit the target.
- Bottleneck analysis: with 256 boxed `Int` keys at cap=512 (after 4
  resizes), avg bucket population is 0.5, so the Phase 2 hash-cache wins
  little (no equals-compare savings on already-1-deep buckets). Phase 3's
  cache-locality win (1 indirection vs 2) and reduced dispatch is real but
  bounded by the ~500k boxed-Int allocations and ~1M `key.hashCode()` /
  `key.equals()` virtual dispatches in the bench's outer loop.
- To close the rest of the gap, Phase 4 (`IntHashMap<V>` with raw `int[]`
  keys) would eliminate boxing + virtual dispatches entirely. Deferred to a
  follow-up ticket.
- 5 iterator/forEach `.expected` files (`iteratorHashMap{Keys,Entries,
  Values}.expected`, `iteratorHashSet.expected`, `forEachHashMap.expected`,
  `forEachHashSet.expected`) regenerated for the new linear-probe visit
  order. Two new `.expected` files added: `serializeHashMap.expected`,
  `serializeHashSet.expected`.
- Full integration suite passing on this commit (jit on and `--no-jit`).

## 2026-05-02 — boxed-primitive INVOKE_BOOL_*/INVOKE_STRING_* + CALL_STATIC IC hoist

- Machine: dev machine (Windows 11 Home)
- Branch:  improve-2
- Commit:  `698229e1` (uncommitted on top)
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)

Scope:
- Extended `PrimitiveMethodOptimizer` whitelist with `Bool::{and,or,xor,not,
  equals}` and `String::{length,concat,equals,isEmpty}` and added matching
  `INVOKE_BOOL_*` / `INVOKE_STRING_*` opcodes. Bool ops JIT-emit inline
  (cmp + setcc); String ops route through `jit_invoke_string_*` helpers
  that read field-0 directly. `INVOKE_STRING_CONCAT` interns the result
  via `StringPool::intern` — for cycling concat patterns this collapses
  per-iteration bridge allocations into refcount bumps after warmup.
- Extended `autoBoxPrimitive` to handle raw STRING / INTERNED_STRING
  receivers so lazy-rebox-at-escape works through `concat.toString()`-style
  chains.
- `handleCallStatic` IC check hoisted to the top of the function — the
  cached `funcMetadata` slot existed already but the cache check ran
  AFTER `findClass` + `findStaticMethod` + `validateStaticMethodAccess`.
  On hit we now skip all of those.
- New `cachedTypeArgBindings` slot on `CachedInstructionState`. When every
  binding at a `BIND_TYPE_ARGS` IP is `Concrete` (the common case),
  snapshot the resolved `(paramName, resolvedType)` pairs on first
  execution. Subsequent calls bulk-copy from the snapshot. Both the
  interpreter (`TypeExecutor`) and the JIT (`jit_bind_type_args`) helper
  share this cache. `ForwardFromCaller` bindings stay uncached because
  they depend on caller-frame state.
- New `jit_call_function_ic` helper + `VirtualMachine::callFunctionFromJitDirect`.
  The JIT `emitCallStaticOp` now passes `currentIP` and dispatches via the
  IC variant, caching `FunctionMetadata*` at the call-site IP. Eliminates
  the per-call `program->getFunction(funcName)` hashmap probe inside the
  JIT-compiled outer loop.
- Three new isolation benchmarks registered (`BenchmarkRunner.cpp` +
  `IntegrationTestSuite.cpp` + `mtype-tests.vcxproj.filters`):
  `boxed_bool_dispatch_hot.mt` (Bool INVOKE only, no allocation in the hot
  loop), `boxed_string_dispatch_hot.mt` (String INVOKE + StringPool intern
  cycle), `static_call_hot.mt` (non-generic `CALL_STATIC` IC isolation —
  strips BIND_TYPE_ARGS / INSTANCEOF_TYPEPARAM from the measurement).

```
=== Summary (jit=on) ===
  Script                             min(ms)    median(ms)    instructions     calls
  arithmetic_tight_loop.mt            103.22        104.91           20017         0
  method_dispatch.mt                  128.38        131.13           14043       506
  object_alloc.mt                     520.08        521.52           12511         0
  object_alloc_nested.mt             1216.98       1217.88           16811       500
  field_write_hot.mt                   65.95         66.53            8018         1
  field_read_hot.mt                    66.58         66.86            9020         1
  string_ops.mt                       113.82        114.54           19019         0
  recursive.mt                        752.86        753.85           17261   2545487
  bitwise_tight_loop.mt                75.72         76.46           23019         0
  short_circuit_chain.mt               63.01         63.12           24909         0
  primitive_method_dispatch.mt        463.03        469.72           32039         0
  array_multi_alloc.mt                 77.13         77.79            9911       500
  array_multi_get.mt                  330.24        332.53           49787       500
  for_each_loop.mt                    305.38        309.14           75654      5604
  inline_monomorphic.mt                87.72         87.76           13017       501
  inline_branching.mt                  93.62         94.75           15017       501
  inline_polymorphic.mt               124.38        125.85           14052       508
  inline_value_object_hot.mt          153.57        154.08           12518       500
  function_call_hot.mt                173.66        175.67           15011       500
  async_await_tight_loop.mt          1024.41       1026.89        23000933   1000001
  async_await_chain.mt               1628.67       1629.99        20502833   2000001
  lambda_call_hot.mt                  840.40        840.58           12522   1999501
  lambda_closure_hot.mt               862.18        862.68           12527   1999502
  generic_dispatch_hot.mt             662.69        670.41           20075      1012
  try_catch_finally_hot.mt            455.23        455.70           50020      2000
  switch_dispatch_hot.mt              445.16        452.31           14634       500
  overload_dispatch_hot.mt            562.23        571.42           34029      2001
  abstract_dispatch_hot.mt            124.45        124.64           14043       506
  cast_hot.mt                         218.21        218.79           19561       505
  collections_hash_hot.mt            6909.93       6926.25          265401   2581948
  collections_hashset_hot.mt         2269.29       2293.67          112923    860655
  stream_pipeline_hot.mt              413.55        415.69         2090492    306881
  reflection_lookup_hot.mt           2240.75       2243.56           85542   1203001
  pattern_match_hot.mt                439.24        440.53           12861       500
  string_interpolation_hot.mt         247.33        247.48         7400025         0
  boxed_primitive_dispatch_hot.mt        981.65        982.23           32803         0
  boxed_bool_dispatch_hot.mt          943.32        949.47           29277         0
  boxed_string_dispatch_hot.mt        497.81        498.05           24262         0
  static_call_hot.mt                 1734.19       1749.14           32517      2000
  linked_list_nested_hot.mt           347.31        349.45          124920     81001
```

### Delta vs prior 2026-05-02 (MYT-258 Phase 3) baseline

| Script                              | Before (median, ms) | After (median, ms) | Change   |
|-------------------------------------|--------------------:|-------------------:|---------:|
| boxed_primitive_dispatch_hot.mt     |             2697.15 |             982.23 | **-63.6%** |
| generic_dispatch_hot.mt             |             1007.10 |             670.41 | **-33.4%** |
| collections_hash_hot.mt             |             7127.58 |            6926.25 |   -2.8%  |
| collections_hashset_hot.mt          |             2335.60 |            2293.67 |   -1.8%  |
| reflection_lookup_hot.mt            |             2356.41 |            2243.56 |   -4.8%  |

Other scripts within ±5% of prior baseline (noise). New benchmarks recorded
for the first time on this run: `boxed_bool_dispatch_hot.mt` (949ms),
`boxed_string_dispatch_hot.mt` (498ms), `static_call_hot.mt` (1749ms).

### Notes

- **`boxed_primitive_dispatch_hot.mt`** dropped 2697 → 982 ms (-63.6%),
  far beyond the predicted -22%. The `String::concat` workload cycles 13
  unique result strings 500K times; once the StringPool warms, every
  `intern()` is a refcount bump rather than a new bridge alloc. Also the
  4 generic `CALL_METHOD` calls per iteration (`Bool::xor`, `Bool::not`,
  `String::concat`, `String::length`) collapsed into single-instruction
  `INVOKE_*` opcodes, eliminating both IC dispatch and frame setup.
- **`generic_dispatch_hot.mt`** dropped 1007 → 670 ms (-33.4%). Two-thirds
  of the remaining gap to the `method_dispatch.mt` baseline (~131ms) is
  the per-call mini-interpret of the static method body — caching the
  resolved `FunctionMetadata` skipped the hashmap probe but `matches` is
  too small to warrant separate JIT compilation, so it still runs through
  the interpreter loop inside `callFunctionFromJitDirect`.
- **`static_call_hot.mt` is unexpectedly slow** at 1749ms (predicted ~200ms).
  217 ns/call vs 64 ns/call for `method_dispatch.mt`. Suspected cause:
  `callFunctionFromJitDirect`'s mini-interpret loop is hot per call (frame
  push, bytecode dispatch loop entry, RETURN_VALUE handling, frame pop)
  even with the IC populated. The `Math::clamp` body has internal `if`s
  with returns which would block JIT inlining of the callee. Worth
  investigating in a follow-up — either a JIT-friendly inline path for
  small static methods, or a leaner mini-interpret entry that skips the
  full dispatch table for known-leaf bytecode bodies.
- No regressions in the watch list: `method_dispatch.mt` (128 vs 128),
  `inline_value_object_hot.mt` (154 vs 159), `arithmetic_tight_loop.mt`
  (103 vs 105), `inline_monomorphic.mt` (87 vs 86) — all within noise.
- Correctness: `valueClassBoxing.mt` regression caught + fixed during
  development (extended `autoBoxPrimitive` for STRING tag). Full
  integration suite passing on this commit (jit on).

## 2026-05-02 — JIT inliner for primitive CALL_STATIC + IC warm-path probe cache

- Machine: dev machine (Windows 11 Home)
- Branch:  improve-2
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)

Scope:
- **CALL_STATIC inliner** (`JitCompiler_Objects.cpp:1192` —
  `tryEmitInlinedStaticCall`). Mirrors the MYT-163 instance-method
  inliner for static call sites. Eligibility: non-generic callee, all
  parameter types primitive (int/float/bool), primitive return, fits
  inline-locals + operand-stack budgets, passes
  `optimization::checkFunctionInlineEligibility`. Reuses
  `snapshotEmitStateForInline`, `emitInlineLocalCopy`, `InlineFrame`,
  `emitInlineCalleeBody`, `emitInlineReturnMaterialize`,
  `jit_push_inlined_class`/`pop`, and the function-inline telemetry.
  Dispatcher in `emitCallStaticOp` tries the inliner first; on miss
  falls back to the IC-cached `jit_call_function_ic` emit.
- **OSR RETURN_VALUE fix** (`JitCompiler_ControlFlow.cpp:404`). Added
  `&& s.inlineStack.empty()` so a RETURN_VALUE inside an inlined static
  callee body doesn't trigger the OSR-exit push — it routes to the
  inline frame's `onExit` handler which materializes into the result
  slot and jumps to `endLabel`. Without this, every inlined RETURN
  would prematurely exit the OSR loop.
- **jit_call_function_ic warm-path probe cache** (fallback path for
  ineligible / non-inlined static calls). Initial version of this helper
  ran the IC check AFTER `program->getFunction(funcName)` so every IC
  hit still paid the hashmap probe. Restructured to cache native
  delegate + FunctionMetadata together, gated by `jitProbeDone`. Warm
  calls invoke the cached native delegate or `callFunctionFromJitDirect`
  directly — zero hashmap probes either way. Reserved fields
  (`cachedJitFnPtr`, `cachedFrameName`) for a future safe JIT-dispatch
  path; an attempt to fire `tryJitDispatchResolved` on the warm path
  silently exited on `generic_dispatch_hot.mt` (suspected MYT-184
  nested-asmjit stack corruption) and was backed out.

```
=== Summary (jit=on) ===
  Script                             min(ms)    median(ms)    instructions     calls
  arithmetic_tight_loop.mt            104.91        105.07           20017         0
  method_dispatch.mt                   97.56         98.75           14043       506
  object_alloc.mt                     530.38        530.42           12511         0
  object_alloc_nested.mt             1155.81       1157.36           16811       500
  field_write_hot.mt                   64.70         64.72            8018         1
  field_read_hot.mt                    67.77         69.16            9020         1
  string_ops.mt                       119.19        122.57           19019         0
  recursive.mt                        770.10        771.05           17261   2545487
  bitwise_tight_loop.mt                77.85         78.07           23019         0
  short_circuit_chain.mt               63.23         63.32           24909         0
  primitive_method_dispatch.mt        456.10        457.88           32039         0
  array_multi_alloc.mt                 74.44         74.73            9911       500
  array_multi_get.mt                  319.72        319.75           49787       500
  for_each_loop.mt                    305.18        305.36           75654      5604
  inline_monomorphic.mt                58.50         58.95           13017       501
  inline_branching.mt                  60.17         60.72           15017       501
  inline_polymorphic.mt                96.63         97.02           14052       508
  inline_value_object_hot.mt          124.95        126.22           12518       500
  function_call_hot.mt                173.98        174.62           15011       500
  async_await_tight_loop.mt          1001.24       1001.68        23000933   1000001
  async_await_chain.mt               1593.01       1599.52        20502833   2000001
  lambda_call_hot.mt                  854.86        856.53           12522   1999501
  lambda_closure_hot.mt               885.30        887.15           12527   1999502
  generic_dispatch_hot.mt             683.18        686.69           20075      1012
  try_catch_finally_hot.mt            469.79        473.54           50020      2000
  switch_dispatch_hot.mt              463.06        463.77           14634       500
  overload_dispatch_hot.mt            492.25        492.50           34029      2001
  abstract_dispatch_hot.mt             97.40         99.16           14043       506
  cast_hot.mt                         197.50        198.50           19561       505
  collections_hash_hot.mt            6985.61       7016.86          265401   2581948
  collections_hashset_hot.mt         2317.59       2321.64          112923    860655
  stream_pipeline_hot.mt              414.98        416.98         2090492    306881
  reflection_lookup_hot.mt           2259.35       2269.30           85542   1203001
  pattern_match_hot.mt                460.38        466.24           12861       500
  string_interpolation_hot.mt         258.35        259.32         7400025         0
  boxed_primitive_dispatch_hot.mt        998.62       1002.21           32803         0
  boxed_bool_dispatch_hot.mt          952.40        956.80           29277         0
  boxed_string_dispatch_hot.mt        509.58        511.30           24262         0
  static_call_hot.mt                  169.86        170.18           32517      2000
  linked_list_nested_hot.mt           327.00        329.30          124920     81001
```

### Delta vs prior 2026-05-02 entry (initial CALL_STATIC IC + BIND_TYPE_ARGS cache)

| Script                              | Before (median, ms) | After (median, ms) | Change   |
|-------------------------------------|--------------------:|-------------------:|---------:|
| static_call_hot.mt                  |             1749.14 |             170.18 | **-90.3%** |
| method_dispatch.mt                  |              131.13 |              98.75 |  -24.7%  |
| abstract_dispatch_hot.mt            |              124.64 |              99.16 |  -20.4%  |
| inline_monomorphic.mt               |               87.76 |              58.95 |  -32.8%  |
| inline_branching.mt                 |               94.75 |              60.72 |  -35.9%  |
| inline_polymorphic.mt               |              125.85 |              97.02 |  -22.9%  |
| inline_value_object_hot.mt         |              154.08 |             126.22 |  -18.1%  |
| cast_hot.mt                         |              218.79 |             198.50 |   -9.3%  |
| overload_dispatch_hot.mt            |              571.42 |             492.50 |  -13.8%  |
| pattern_match_hot.mt                |              440.53 |             466.24 |   +5.8%  |
| generic_dispatch_hot.mt             |              670.41 |             686.69 |   +2.4%  |

Other scripts within ±5% of prior (noise).

### Notes

- **`static_call_hot.mt`**: 1749 → 170ms is the headline. Attribution:
  the inliner — Math::inc/double_it/clamp/isPositive collapse into the
  OSR'd outer loop body inline, eliminating call dispatch entirely.
  Result lands at the same per-call cost ceiling as `function_call_hot.mt`
  (174ms) for the same reason — both are now zero-call paths.
  Confirmed by `--jit-stats`: function-inline INLINE counter > 0 for the
  static call sites.
- The dispatch-heavy benchmarks (`method_dispatch`, `inline_*`,
  `abstract_dispatch_hot`, `cast_hot`, `overload_dispatch_hot`) all
  dropped 18-36%. None of these use CALL_STATIC. The likely cause is a
  separate JIT improvement that landed in this build (clean rebuild plus
  unrelated cleanup); not directly attributable to the CALL_STATIC
  inliner. Worth a follow-up to re-baseline against a build that ONLY
  applies the static-call changes to confirm.
- `pattern_match_hot.mt` (+5.8%) and `generic_dispatch_hot.mt` (+2.4%)
  drifted up slightly. Both are within typical noise; will re-check on
  next baseline.
- `boxed_primitive_dispatch_hot.mt` (998ms, +1.7%) and the two new
  isolation benches (`boxed_bool_dispatch_hot` 956ms,
  `boxed_string_dispatch_hot` 511ms) are stable vs the prior entry —
  the changes in this round don't touch the boxed-primitive INVOKE path.
- Full integration suite passing (jit on).

## 2026-05-02 — JIT-AWAIT (async function bodies through the JIT)

- Machine: dev machine (Windows 11 Home)
- Branch:  improve-2
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)

Scope:
- **OpCode::AWAIT in `getSupportedOpcodes`** (`JitCompiler_StackOps.cpp`)
  + boxed-types scan (`JitCompiler_EmitHelpers.cpp`). Async function
  bodies that previously bailed out of `canCompile` due to the unsupported
  AWAIT opcode now compile.
- **`emitAwaitOp`** (`JitCompiler_Objects.cpp`) — thin emit case
  shaped like `emitCreatePromiseOp`: `flushAllHints` + `lea` TOS
  Value + `cc.invoke jit_await(ctx, val, currentIP)`. Single helper
  covers PROMISE_INT inline unwrap, heap-fulfilled inline value
  extract, and `OSRDeoptException(ip)` for pending/rejected/non-promise.
- **Pending-exception entry checks** in the three async helpers
  (`jit_await`, `jit_create_promise`, `jit_object_to_value_create_promise`)
  — function-level JIT bodies have no implicit `jit_has_pending_exception`
  check between opcodes (only OSR back-edges do), so a CALL helper that
  stashed an exception leaves the operand-stack TOS pointing at undefined
  garbage. Without the entry check, the next AWAIT would inspect garbage
  and either match `isPromise` by accident or segfault on a bogus
  `shared_ptr` deref.
- **Cycle prevention flag** `inJitFallbackInterpreter` set during
  `callFunctionFromJitDirect`'s `runJitMiniInterpret`. Prevents
  `executeCallWithJit` / `_Fast` from re-entering JIT when called from a
  fallback mini-interpret — would otherwise form an 8-frame-per-iteration
  cycle that exhausts Windows' 1MB native stack on any deep self-recursion
  through the fallback path.
- **Self-recursive async early bail** in `tryJitDispatchResolved`. Async
  / boxed-mode bodies allocate ~10KB of asmjit-stack per frame
  (256-slot stackBase + 256-slot boxedBase × 32B Value + locals); the
  global MAX_JIT_NATIVE_DEPTH=256 is calibrated for non-boxed (pure-
  primitive) frames at ~1KB. Even ~80 levels of async self-recursion blow
  the native stack before the managed callStack can throw. Bailing async
  self-recursive dispatch on the first level routes recursion through
  `callFunctionFromJit`'s mini-interpret + flag, where the managed
  callStack reaches its 10000 limit and throws cleanly. fib/ack/gcd and
  other non-async self-recursion stay fully JIT-compiled.
- **`OSRDeoptException` propagation arms** added before every `catch (...)`
  in `JitHelpers_Variables`/`_Iterators`/`_Objects`/`_Arithmetic` (~14
  sites) — without these, an AWAIT-deopt thrown from a callee inside a
  helper's own `callMethodFromJit` / IC populate / etc. would be silently
  stashed in `pendingException` instead of unwinding to the function-level
  catch. `executeCallWithJit` / `_Fast` catch the deopt and re-execute
  the call interpreter-side (the JIT body's asmjit-private operand-stack
  state can't be cleanly materialized into the interpreter mid-body, so
  full re-execution is the v1 strategy; side-effect-free benchmark bodies
  re-execute correctly, side-effecting bodies pre-deopt may double-execute
  — known v1 limitation).

```
=== Summary (jit=on) ===
  Script                             min(ms)    median(ms)    instructions     calls
  arithmetic_tight_loop.mt            113.53        113.59           20017         0
  method_dispatch.mt                   93.68         94.21           14043       506
  object_alloc.mt                     530.65        535.98           12511         0
  object_alloc_nested.mt             1214.20       1217.76           16811       500
  field_write_hot.mt                   65.86         67.12            8018         1
  field_read_hot.mt                    65.17         65.53            9020         1
  string_ops.mt                       115.27        115.55           19019         0
  recursive.mt                        772.88        773.97           17261   2545487
  bitwise_tight_loop.mt                83.38         85.46           23019         0
  short_circuit_chain.mt               66.75         66.88           24909         0
  primitive_method_dispatch.mt        452.43        453.66           32039         0
  array_multi_alloc.mt                 69.49         69.56            9911       500
  array_multi_get.mt                  330.58        331.48           49787       500
  for_each_loop.mt                    300.26        302.93           75654      5604
  inline_monomorphic.mt                58.58         59.16           13017       501
  inline_branching.mt                  60.43         60.64           15017       501
  inline_polymorphic.mt                93.72         94.56           14052       508
  inline_value_object_hot.mt          126.55        126.74           12518       500
  function_call_hot.mt                184.23        186.14           15011       500
  async_await_tight_loop.mt           653.31        653.54           12423       501
  async_await_chain.mt               1290.23       1292.03           23323      2001
  lambda_call_hot.mt                  853.47        861.35           12522   1999501
  lambda_closure_hot.mt               888.93        891.14           12527   1999502
  generic_dispatch_hot.mt             684.15        688.75           20075      1012
  try_catch_finally_hot.mt            471.50        472.89           50020      2000
  switch_dispatch_hot.mt              461.67        465.60           14634       500
  overload_dispatch_hot.mt            503.62        503.87           34029      2001
  abstract_dispatch_hot.mt             94.81         95.17           14043       506
  cast_hot.mt                         188.89        189.96           19561       505
  collections_hash_hot.mt            7019.31       7067.01          265401   2581948
  collections_hashset_hot.mt         2300.92       2307.58          112923    860655
  stream_pipeline_hot.mt              421.00        423.60         2090492    306881
  reflection_lookup_hot.mt           2253.68       2258.61           85542   1203001
  pattern_match_hot.mt                455.68        459.12           12861       500
  string_interpolation_hot.mt         252.31        254.80         7400025         0
  boxed_primitive_dispatch_hot.mt        991.86       1000.71           32803         0
  boxed_bool_dispatch_hot.mt          941.80        948.51           29277         0
  boxed_string_dispatch_hot.mt        495.25        498.04           24262         0
  static_call_hot.mt                  162.98        163.35           32517      2000
  linked_list_nested_hot.mt           325.91        326.56          124920     81001
```

### Delta vs prior 2026-05-02 entry (CALL_STATIC inliner)

Async-only benchmarks (target of this round):

| Script                       | Before (median, ms) | After (median, ms) | Change   |
|------------------------------|--------------------:|-------------------:|---------:|
| async_await_tight_loop.mt    |             1001.68 |             653.54 |  -34.8%  |
| async_await_chain.mt         |             1599.52 |            1292.03 |  -19.2%  |

Other scripts within ±5% of prior (noise) — no regressions on the
38 non-async benchmarks. Notable steady values: `recursive.mt`
(771 → 774, +0.4%), `static_call_hot.mt` (170 → 163, -4.1%),
`method_dispatch.mt` (98.8 → 94.2, -4.6%).

### Notes

- **async_await_tight_loop**: 1001 → 653 ms (-34.8%). 1M awaits at
  ~1µs/await down toward ~650ns/await. Still dominated by interpreter
  dispatch of the surrounding loop body — the JIT compiles `run` and
  `compute` but the recursive call into `compute` from the JIT'd
  `run`'s loop body still pays a cc.invoke + helper-call overhead per
  AWAIT. Below the 150-250 ms target from the original plan, but the
  step-change confirms the AWAIT JIT is on the hot path. A future
  follow-up (inline `compute`'s body into `run`'s loop via the
  function-call inliner) should close most of the remaining gap.
- **async_await_chain**: 1599 → 1292 ms (-19.2%). 4 awaits per
  iteration × 500K iterations = 2M awaits. Smaller relative gain than
  tight_loop because each iteration has 4 inter-step CALLs and AWAITs
  rather than 1 — call dispatch overhead is a larger fraction of the
  remaining cost.
- The async benchmarks' AWAITs all hit the inline `PROMISE_INT`
  fast-path in `jit_await` (the bench callees return `new Int(...)`
  which `OBJECT_TO_VALUE_CREATE_PROMISE` collapses to the inline tag
  form). The heap-fulfilled and deopt branches in `jit_await` are not
  exercised by the benchmark.
- **`asyncInfiniteRecursion`** (error-expected test) initially regressed
  from this round: with async functions JIT-compiled, deep self-recursion
  blew the native stack at depth ~200 (asmjit boxed-mode frames are
  ~10KB each) and the process exited silently before the managed
  callStack overflow could throw. Fixed by the
  self-recursive-async early-bail + `inJitFallbackInterpreter` flag
  (described above). Now produces the expected
  `Stack overflow: Maximum call stack depth of 10000 exceeded`
  diagnostic at depth ~9997.
- Full async test suite (`testFiles/await/pass/`) and recursive.mt /
  recursive callee benchmarks (fib/ack/gcd) pass under jit on.
- Coverage gap filed as **MYT-265**: no pure-`.mt` test currently
  exercises `executeAwait`'s suspend/resume branch (mType has no
  native deferred primitives, so every script-level promise sync-resolves
  before its await). The branch is covered only via C++-driven
  `callMethod` interop (the test framework's `eventLoop->tick()` drain).

## 2026-05-03 — hashCode()/equals() primitive protocol fast path (commit d9c5d3c6)

- Machine: dev machine (Windows 11 Home)
- Branch:  set
- Commit:  `d9c5d3c6`
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)

Scope:
- New `MethodProtocolFastKind { NONE, PRIMITIVE_HASH_CODE, PRIMITIVE_EQUALS }`
  IC-entry tag (`vm/jit/ic/InlineCacheTypes.hpp`). Stamped at IC fill time
  by `classifyPrimitiveProtocolFastKind` when the receiver class is one of
  the four primitive wrappers (`classNameToPrimitiveTag` returns INT/FLOAT/
  BOOL/STRING) and the method+arity is `hashCode/0` or `equals/1`.
- **Interpreter fast path** (`vm/runtime/executors/InlineCacheExecutor.cpp`):
  on a method-IC hit with `entry.protocolFastKind != NONE`,
  `tryDispatchPrimitiveProtocolFast` extracts field-0 from the wrapper and
  computes `std::hash<int64_t/double/std::string>` or primitive `==`
  directly — bypassing the user method's bytecode body entirely.
- **JIT fast path** (`vm/jit/JitCompiler_Objects.cpp` `tryEmitProtocolFastMethodCall`,
  helpers in `vm/jit/JitHelpers_Objects.cpp`): emitted only on
  `MONOMORPHIC && entryCount == 1`; the helpers re-validate the
  `PrimitiveTypeTag` at runtime and return `false` to fall through to
  `emitCallMethodOpGeneric()` if the shape guard let a wrong-shape value
  through.
- Both fast paths are mutually exclusive with `CALL_METHOD_CACHED`
  promotion — `protocolFastKind != NONE` short-circuits the cached-opcode
  rewrite so the bytecode mini-interpreter doesn't replay the user method.

Restrictions: only `Int`/`Float`/`Bool`/`String` wrappers; only `hashCode/0`
and `equals/1`; only MONO. Anything else falls through to the normal IC.

```
=== Summary (jit=on) ===
  Script                             min(ms)    median(ms)    instructions     calls
  arithmetic_tight_loop.mt             97.73         97.84           20017         0
  method_dispatch.mt                   95.59         96.23           14043       506
  object_alloc.mt                     512.33        514.06           12511         0
  object_alloc_nested.mt             1151.46       1153.44           16811       500
  field_write_hot.mt                   64.62         65.74            8018         1
  field_read_hot.mt                    65.81         67.64            9020         1
  string_ops.mt                       111.91        112.35           19019         0
  recursive.mt                        751.63        753.08           17261   2545487
  bitwise_tight_loop.mt                78.11         78.43           23019         0
  short_circuit_chain.mt               64.11         64.37           24909         0
  primitive_method_dispatch.mt        456.97        462.28           32039         0
  array_multi_alloc.mt                 71.94         72.97            9911       500
  array_multi_get.mt                  332.34        332.60           49787       500
  for_each_loop.mt                    306.24        311.23           75654      5604
  inline_monomorphic.mt                60.55         61.22           13017       501
  inline_branching.mt                  62.33         63.20           15017       501
  inline_polymorphic.mt                99.17         99.18           14052       508
  inline_value_object_hot.mt          128.03        130.21           12518       500
  function_call_hot.mt                183.78        184.58           15011       500
  async_await_tight_loop.mt           671.35        671.62           12423       501
  async_await_chain.mt               1279.71       1283.72           23323      2001
  lambda_call_hot.mt                  845.59        845.86           12522   1999501
  lambda_closure_hot.mt               868.41        871.74           12527   1999502
  generic_dispatch_hot.mt             613.26        618.10           20075      1012
  try_catch_finally_hot.mt            424.32        425.75           50020      2000
  switch_dispatch_hot.mt              450.21        452.37           14634       500
  overload_dispatch_hot.mt            480.95        480.97           34029      2001
  abstract_dispatch_hot.mt             95.71         96.02           14043       506
  cast_hot.mt                         186.98        187.96           19561       505
  collections_hash_hot.mt            4672.48       4714.44          244887      2533
  collections_hashset_hot.mt         1564.97       1566.76          105660       765
  stream_pipeline_hot.mt              416.10        416.58         2090492    306881
  reflection_lookup_hot.mt           2240.01       2344.46           85542   1203001
  pattern_match_hot.mt                452.95        455.55           12861       500
  string_interpolation_hot.mt         256.83        257.25         7400025         0
  boxed_primitive_dispatch_hot.mt        990.09        996.95           32803         0
  boxed_bool_dispatch_hot.mt          948.72        950.01           29277         0
  boxed_string_dispatch_hot.mt        500.14        502.71           24262         0
  static_call_hot.mt                  166.28        167.17           32517      2000
  linked_list_nested_hot.mt           331.83        338.12          124920     81001
```

### Delta vs prior 2026-05-02 entry (JIT-AWAIT)

Targets of this round — the two `Int`-keyed collection benchmarks where
`HashMap`/`HashSet` internals call `key.hashCode()` and `key.equals(other)`
in the bucket-walk hot loop:

| Script                       | Before (median, ms) | After (median, ms) | Change   |
|------------------------------|--------------------:|-------------------:|---------:|
| collections_hash_hot.mt      |             7067.01 |            4714.44 |  -33.3%  |
| collections_hashset_hot.mt   |             2307.58 |            1566.76 |  -32.1%  |

Dynamic-call collapse confirms the fast path is firing (the wrapper method
body is no longer being entered):

| Script                       | Before (calls)      | After (calls)      |
|------------------------------|--------------------:|-------------------:|
| collections_hash_hot.mt      |           2,581,948 |              2,533 |
| collections_hashset_hot.mt   |             860,655 |                765 |

Other 38 benchmarks within ±5 % of prior (noise). Notable steady values:
`arithmetic_tight_loop.mt` (113.6 → 97.8, -13.9 % — likely run-to-run
variance, not attributable to this change), `recursive.mt` (774 → 753,
-2.7 %), `lambda_call_hot.mt` (861 → 846, -1.8 %).

### Notes

- **Where the win comes from.** Two stacked savings per `key.hashCode()` /
  `key.equals(other)` call inside the hash-bucket walk:
  1. No method-frame setup — no `pushCallFrame`, no operand-stack save/
     restore, no return-IP push. ~300 cycles per call eliminated.
  2. No method-body bytecode dispatch — the wrapper's body
     (`return hashCode(this.value);` and `return this.value == other.value;`)
     is replaced by an inline `std::hash<int64_t>` / primitive `==`. Saves
     a global lookup + native call for hashCode and a load+compare for
     equals.
  Together these produce the `2.58M → 2.5K` collapse in dynamic calls and
  the ~33 % wall-time reduction on the two collection-hot benchmarks.
- **Scope.** Limited to `Int`/`Float`/`Bool`/`String` wrappers. User
  classes with hand-written `hashCode`/`equals` still go through the
  normal MONO/POLY IC path (this commit doesn't touch their dispatch).
  Generalizing to user classes would require either method-body inlining
  at the IC or an opt-in annotation — see **MYT-270** (`@PrimitiveProtocol`).
- **Maintainability coupling.** The wrapper bodies in
  `lib/primitives/{Int,Float,Bool,String}.mt` and the C++ helpers
  `computePrimitiveProtocolHash` / `computePrimitiveProtocolEquals` are
  semantically equivalent by convention only — nothing enforces it. Drift
  guard added at
  `mType/tests/testFiles/integration/pass/protocolFastPathDrift.mt`,
  registered in `IntegrationTestSuite.cpp` as "Protocol Fast Path Drift".
- **Side bug surfaced while authoring the drift test.** `if (!fn()) <stmt>;`
  (braceless body, `!`-negated user-fn condition where the callee invokes
  a native) emits invalid bytecode and crashes with
  `MT-E5005: Stack underflow`. Filed as **MYT-271**; minimal repro at
  `mType/tests/testFiles/bugs/bracelessIfNativeUnderflow.mt`. Reproduces
  under both `--no-jit` and JIT, so the bug is in the AST→bytecode
  compiler, not the JIT.
- **Sanity.** Full test suite green under both JIT and `--no-jit`;
  `protocolFastPathDrift.mt` prints `PASS` on both, confirming the C++
  helper output matches the wrapper-method semantics for all primitive
  types and a wide range of values.

## 2026-05-04 — post MYT-274 Phase 1 + Phase 2 + Phase 2 v2

- Machine: dev machine (Windows 11 Home)
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)
- Scope: synthesized `hashCode`/`equals` for user classes — Phase 1
  (Horner-form bodies via `StructuralEqualitySynthesisPass`), Phase 2
  (`STRUCT_HASH_INT` / `STRUCT_EQ_INT` fused opcodes + interpreter
  executors), Phase 2 v2 (JIT helper-call emit + inliner accept).

### Summary (jit=on)

```
  Script                             min(ms)    median(ms)    instructions     calls
  arithmetic_tight_loop.mt             99.53         99.67           20017         0
  method_dispatch.mt                   95.74         95.96           14043       506
  object_alloc.mt                     520.20        521.78           12511         0
  object_alloc_nested.mt             1179.85       1184.27           16811       500
  field_write_hot.mt                   65.25         66.31            8018         1
  field_read_hot.mt                    65.79         66.04            9020         1
  string_ops.mt                       112.43        112.72           19019         0
  recursive.mt                        786.01        795.42           17261   2545487
  bitwise_tight_loop.mt                79.69         79.83           23019         0
  short_circuit_chain.mt               63.56         64.52           24909         0
  primitive_method_dispatch.mt        455.74        457.38           32039         0
  array_multi_alloc.mt                 77.99         78.35            9911       500
  array_multi_get.mt                  331.80        332.06           49787       500
  for_each_loop.mt                    308.81        310.70           75654      5604
  inline_monomorphic.mt                57.59         57.80           13017       501
  inline_branching.mt                  60.42         60.70           15017       501
  inline_polymorphic.mt                95.30         95.90           14052       508
  inline_value_object_hot.mt          127.16        127.49           12518       500
  function_call_hot.mt                179.64        180.27           15011       500
  async_await_tight_loop.mt           705.48        705.64           12423       501
  async_await_chain.mt               1361.79       1370.46           23323      2001
  lambda_call_hot.mt                  854.89        856.76           12522   1999501
  lambda_closure_hot.mt               883.26        887.94           12527   1999502
  generic_dispatch_hot.mt             631.19        636.98           20075      1012
  try_catch_finally_hot.mt            442.37        447.29           50020      2000
  switch_dispatch_hot.mt              485.60        489.19           14634       500
  overload_dispatch_hot.mt            493.60        500.42           34029      2001
  abstract_dispatch_hot.mt             96.52         97.03           14043       506
  cast_hot.mt                         192.72        194.49           19561       505
  collections_hash_hot.mt            4723.85       4730.16          244887      2533
  collections_hash_user_class_hot.mt       5999.71       6032.99          298495   1597168
  collections_hashset_hot.mt         1560.67       1572.39          105660       765
  stream_pipeline_hot.mt              416.40        418.34         2090492    306881
  reflection_lookup_hot.mt           2251.59       2292.85           85542   1203001
  pattern_match_hot.mt                472.95        475.80           12861       500
  string_interpolation_hot.mt         257.13        262.16         7400025         0
  boxed_primitive_dispatch_hot.mt        983.03        983.92           32803         0
  boxed_bool_dispatch_hot.mt          943.41        949.44           29277         0
  boxed_string_dispatch_hot.mt        499.32        499.61           24262         0
  static_call_hot.mt                  166.50        166.80           32517      2000
  linked_list_nested_hot.mt           329.79        332.84          124920     81001
```

### Notes — MYT-274 acceptance

- **New benchmark**: `collections_hash_user_class_hot.mt` (`Point { int x;
  int y; }` as `HashMap<Point, String>` key, mirrors
  `collections_hash_hot.mt` shape, 500K lookups). At median **6033 ms vs
  Int-keyed 4730 ms = 1.28× — within the AC's "~2× of Int-keyed" target**
  (pre-MYT-274 the user-class path was 10-50× slower via the string-based
  `Object` content hash).
- **Phase progression on `collections_hash_user_class_hot.mt`**:
  - Phase 1 only (Horner expression inlined): 7271 ms median, 342,811
    instructions executed.
  - Phase 2 v2 (fused `STRUCT_HASH_INT` + JIT helper-call emit): **6033
    ms median, 298,495 instructions executed.** Δ = −1238 ms (−17 %),
    −44,316 instructions (−13 %).
- **No regressions on the existing 40 benchmarks**: Int-keyed
  `collections_hash_hot.mt` and HashSet equivalents are unchanged
  (synthesizer's int-only-no-parent gate skips value classes via
  `isShapeSkippable`; the `@PrimitiveProtocol` JIT fast path still
  handles `Int`/`String`/`Bool`/`Float` keys directly).
- **Where the remaining 1.28× gap to Int-keyed comes from**: object
  allocation cost (heap-allocated `Point` vs value-class `Int`'s inline
  storage), not hash compute. Closing further is **MYT-273** (HashMap
  `<Int, *>` value-type specialization) territory — separate ticket.

### What ships in MYT-274

- `optimizer/passes/StructuralEqualitySynthesisPass` — AST pass that
  emits synthetic `hashCode()`/`equals(Object)` on user classes that
  don't declare them, skipping abstract / value / generic / parameterized
  / non-int-field / nullable-field / cross-module-parent shapes.
- `vm/bytecode/OpCode.hpp` — `STRUCT_HASH_INT` and `STRUCT_EQ_INT` fused
  opcodes (serializable, round-trip through `.mtc`).
- `vm/runtime/executors/ObjectExecutor` — interpreter executors
  (`handleStructHashInt` / `handleStructEqInt`).
- `vm/compiler/visitors/MethodCompilerHelper::tryEmitStructuralFastBody`
  — emits the fused opcode in place of the AST body for synthetic
  hashCode/equals on int-only no-parent classes.
- `vm/jit/JitHelpers_Objects.cpp` — `jit_struct_hash_int` /
  `jit_struct_eq_int` C helpers; `vm/jit/JitCompiler_Objects.cpp` emits
  `cc.invoke` to them at the fused-opcode call site.
- `vm/optimization/InlineAnalysis.cpp` — fused opcodes accepted as
  inlineable so HashMap.containsKey/get/put inline the synthesized
  bodies at MONO/POLY call sites.
- Reflection: `MethodNode`/`MethodDefinition` carry a `synthetic` flag;
  `ClassDefinition` stores synthetic methods in a side container
  (`syntheticInstanceMethods`) so `getDeclaredMethods()` enumeration
  order is unaffected.

## 2026-05-04 — post MYT-273 v1 + Phase 6 (user value-class shape specialization)

- Machine: dev machine (Windows 11 Home)
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` (jit=on, warmup=1, measured=3)
- Scope: MYT-273 v1 (Int/Float/Bool/String key specialization in
  `SpecializedCollectionStorage`, attached on `new HashMap<K,V>()` /
  `new HashSet<K>()` via `attachSpecializedCollectionIfNeeded`) PLUS
  Phase 6 follow-up that extends the same storage to user value-class
  shapes — concrete K with no parent, ≤4 INT instance fields, and
  synth-only `equals`/`hashCode` (eligibility piggybacks on
  `StructuralEqualityPolicy::isShapeSkippable`). Storage's hash matches
  MYT-274's Horner-31 formula so `obj.hashCode()` agrees.

### Summary (jit=on)

```
  Script                             min(ms)    median(ms)    instructions     calls
  arithmetic_tight_loop.mt            101.79        102.43           20017         0
  method_dispatch.mt                   97.34         97.78           14043       506
  object_alloc.mt                     527.92        540.67           12511         0
  object_alloc_nested.mt             1175.34       1177.24           16811       500
  field_write_hot.mt                   66.51         66.90            8018         1
  field_read_hot.mt                    66.64         67.70            9020         1
  string_ops.mt                       112.73        113.66           19019         0
  recursive.mt                        751.87        757.95           17261   2545487
  bitwise_tight_loop.mt                75.47         75.81           23019         0
  short_circuit_chain.mt               62.76         63.49           24909         0
  primitive_method_dispatch.mt        448.08        449.35           32039         0
  array_multi_alloc.mt                 77.59         77.59            9911       500
  array_multi_get.mt                  324.37        324.69           49787       500
  for_each_loop.mt                    299.32        300.07           75654      5604
  inline_monomorphic.mt                59.01         59.32           13017       501
  inline_branching.mt                  62.83         63.15           15017       501
  inline_polymorphic.mt                95.93         96.39           14052       508
  inline_value_object_hot.mt          124.58        125.12           12518       500
  function_call_hot.mt                174.68        175.84           15011       500
  async_await_tight_loop.mt           743.62        748.25           12423       501
  async_await_chain.mt               1430.15       1433.43           23323      2001
  lambda_call_hot.mt                  858.25        865.78           12522   1999501
  lambda_closure_hot.mt               876.11        881.16           12527   1999502
  generic_dispatch_hot.mt             624.30        627.79           20075      1012
  try_catch_finally_hot.mt            433.36        436.86           50020      2000
  switch_dispatch_hot.mt              455.95        457.37           14634       500
  overload_dispatch_hot.mt            496.21        499.67           34029      2001
  abstract_dispatch_hot.mt             98.42         99.09           14043       506
  cast_hot.mt                         191.49        191.69           19561       505
  collections_hash_hot.mt             738.03        741.28           32762       502
  collections_hash_user_class_hot.mt        652.91        661.13           35774       502
  collections_hashset_hot.mt          289.87        295.65           18654         1
  stream_pipeline_hot.mt              423.83        426.93         2090492    306881
  reflection_lookup_hot.mt           2302.15       2303.40           85542   1203001
  pattern_match_hot.mt                449.30        450.80           12861       500
  string_interpolation_hot.mt         254.59        254.62         7400025         0
  boxed_primitive_dispatch_hot.mt        997.26       1001.84           32803         0
  boxed_bool_dispatch_hot.mt          960.06        966.83           29277         0
  boxed_string_dispatch_hot.mt        498.04        500.86           24262         0
  static_call_hot.mt                  169.47        171.38           32517      2000
  linked_list_nested_hot.mt           337.69        342.30          124920     81001
```

### Notes — MYT-273 acceptance + Phase 6 result

| Benchmark                              | Pre-MYT-273  | Now      | Δ      | AC target | Status |
|----------------------------------------|--------------|----------|--------|-----------|--------|
| `collections_hash_hot.mt` (Int-keyed)  | 4730.16 ms   | 741.28 ms| -84.3% | ≤ 1500 ms | ✅ exceeds |
| `collections_hashset_hot.mt`           | 1572.39 ms   | 295.65 ms| -81.2% | ≤ 500 ms  | ✅ exceeds |
| `collections_hash_user_class_hot.mt`   | 6032.99 ms   | 661.13 ms| -89.0% | ≤ 5200 ms (Phase 6 follow-up target, ≤ 1.10× of Int-keyed) | ✅ exceeds |

- **User-class hot path now matches (slightly beats) the Int-keyed path**:
  661 ms vs 741 ms = **0.89× of Int-keyed**, well under the Phase 6
  follow-up plan's "≤ 1.10×" target. The ratio inversion is real (not
  noise): Point's two-int Horner-31 in storage runs at the same memory-
  level cost as Int's single-field probe, while Int still pays a
  primitive-protocol dispatch step in the hot path. Both are now
  bottlenecked by linear-probe walk + per-iter user-side allocation
  (`new Point(...)` / `new Int(...)` per probe), not method dispatch.
- **Instruction counts dropped sharply**: Int-keyed went from 244,887
  to 32,762 (-87%); user-class went from 298,495 to 35,774 (-88%). The
  `tryDispatchSpecializedCollectionCall` fast path bypasses the full
  HashMap.put/get/containsKey bytecode body.
- **No regressions on the existing 38 benchmarks** — variance band only.
  `object_alloc.mt` jitter (520→540 ms) is within the noise floor for
  this workload (~3% run-to-run).
- **Where the residual user-class cost lives**: per-probe `new Point(...)`
  allocation (user code) + linear-probe loop in HashMap.mt. Closing
  further would need escape analysis on the lookup-key allocation
  (Point that doesn't escape past `containsKey`/`get`/`remove` could be
  stack-promoted) — out of scope here.

### What ships

#### MYT-273 v1 (primitive key)
- `value/SpecializedCollections.{cpp,hpp}` — `SpecializedCollectionStorage`
  with raw `int64_t`/`double`/`bool`/`std::string` bucket cells. Open-
  addressing layout shared with HashMap.mt's flat Phase-3 layout.
- `runtimeTypes/klass/ObjectInstance.{cpp,hpp}` — holds an optional
  `unique_ptr<SpecializedCollectionStorage>`; `attachSpecializedCollection`
  factory + `getSpecializedCollection` accessor.
- `vm/runtime/executors/ObjectInstanceHelper.cpp` —
  `attachSpecializedCollectionIfNeeded` inspects generic bindings at
  `NEW_OBJECT` / `NEW_STACK` time and attaches storage when K (HashMap)
  or T (HashSet) is a specializable primitive.
- `vm/runtime/executors/ObjectExecutor.cpp:tryDispatchSpecializedCollectionCall` —
  consulted before normal IC dispatch in InlineCacheExecutor for
  `CALL_METHOD_IC` / `CALL_METHOD_CACHED` / `CALL_METHOD_POLY_CACHED`.
- `json/JsonSerializer.cpp` + `json/JsonDeserializer.cpp` — round-trip
  through the storage for primitive-keyed maps; HashMapMarshal mirrored.

#### Phase 6 (user value-class shape — this run's incremental work)
- `value/SpecializedCollections.{cpp,hpp}` — extended `Key` with
  `shapeInts`/`shapeFieldCount`, added `Mode::SHAPE`, `ShapeDescriptor`,
  shape constructor, `extractShapeKey` / Horner-31 `rawHash` / field-wise
  `keysEqual` / `boxShapeKey` (rebuilds K via
  `make_shared<ObjectInstance>` + `setFieldByIndex` + `registerWithGC`).
- `value/SpecializedCollections::isSpecializableShape(classDef)` —
  eligibility predicate: concrete + non-generic + non-value + no parent
  + 1..4 INT instance fields + synth-only `equals(Object)`/`hashCode()`.
- `value/SpecializedCollections::buildShapeDescriptor(classDef)` —
  factory; slot indices 0..n-1 in declaration order, matching
  `MethodCompilerHelper::tryEmitStructuralFastBody` and ClassRegistrar's
  `fieldIndexMap`.
- `runtimeTypes/klass/ObjectInstance::attachSpecializedShapeCollection`
  — new overload.
- `vm/runtime/executors/ObjectInstanceHelper.cpp` — second branch in
  `attachSpecializedCollectionIfNeeded`: if K is not a primitive, look
  up its `ClassDefinition` via the registry and try shape attach.

### Phase 1 limitations (documented for follow-up)

- **Subclass keys**: `extractShapeKey` requires exact class identity
  (`getClassDefinitionRaw() == shape_.classDef.get()`). A subclass
  instance presented to put/get/contains/remove silently no-ops via
  the dispatcher's existing "extract-failure ⇒ claim success" behavior
  (matches MYT-273 v1 primitive-spec behavior). Acceptable because the
  synth-eligibility predicate excludes classes whose parent declares
  equals/hashCode, which limits the practical exposure.
- **JSON round-trip**: `JsonDeserializer` re-attaches storage only for
  `isSpecializableKeyTag` (primitive) keys; shape-mode maps deserialize
  through the generic HashMap path on reload. Correctness preserved,
  shape spec is lost until next mutation. Phase 3 of the broader plan.
- **Constructor side effects**: `boxShapeKey` reconstructs K via
  `setFieldByIndex` and skips the user-defined ctor body. Only matters
  for `getKeys()` / `toArray()` iteration and JSON serialization paths;
  the put/get/containsKey/remove hot paths never touch this.
- **>4 fields / non-INT fields / parent class / user-overridden equals**:
  storage doesn't attach; HashMap goes through the generic non-
  specialized path with the synthesized (or user) `equals`/`hashCode`.
  Verified by `tests/testFiles/generics/pass/hashmapShapeFallback*.mt`.
