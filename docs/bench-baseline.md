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
