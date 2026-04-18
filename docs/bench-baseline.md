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
