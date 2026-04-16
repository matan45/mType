# mType Interpreter Baseline

Committed min wall-clock numbers from `mType.exe --benchmark`.
See `docs/benchmarks.md` for how to update.

> **Note:** the first entry below is a placeholder ready to be filled in
> after the first dev-machine run. Replace the `...` values and iteration
> counts that land outside the 1-5s target band; commit the result.

## YYYY-MM-DD — initial baseline (to be filled in)

- Machine: `<CPU, RAM cores, OS>`
- Commit:  `<sha>`
- Build:   Release x64, MSVC v145
- Invocation: `mType.exe --benchmark` for jit=on, `mType.exe --benchmark --no-jit` for jit=off

| Script                    | Wall min (ms) | exec (ms) | Instructions | Calls | JIT |
|---------------------------|--------------:|----------:|-------------:|------:|-----|
| arithmetic_tight_loop.mt  |           ... |       ... |          ... |   ... | on  |
| arithmetic_tight_loop.mt  |           ... |       ... |          ... |   ... | off |
| method_dispatch.mt        |           ... |       ... |          ... |   ... | on  |
| method_dispatch.mt        |           ... |       ... |          ... |   ... | off |
| object_alloc.mt           |           ... |       ... |          ... |   ... | on  |
| object_alloc.mt           |           ... |       ... |          ... |   ... | off |
| string_ops.mt             |           ... |       ... |          ... |   ... | on  |
| string_ops.mt             |           ... |       ... |          ... |   ... | off |
| recursive.mt              |           ... |       ... |          ... |   ... | on  |
| recursive.mt              |           ... |       ... |          ... |   ... | off |

### Sanity outputs (must match on re-run for same commit)

- `arithmetic_tight_loop.mt`: `intSum=...` and float sum line
- `method_dispatch.mt`: `acc=...`
- `object_alloc.mt`: `total=...`
- `string_ops.mt`: `concat_iters=20000 matches=1000000`
- `recursive.mt`: `fib32=2178309 ack38=2045 gcdSum=...`
