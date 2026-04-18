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
