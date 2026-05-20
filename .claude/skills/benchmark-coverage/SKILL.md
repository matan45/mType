---
name: benchmark-coverage
description: Authoring new benchmarks under mType/tests/testFiles/benchmarks/ and auditing coverage gaps across interpreter opcodes, JIT helpers, IC paths, and OSR triggers. Use when the user asks to add a benchmark, write a perf test for a feature, audit coverage of a JIT/runtime subsystem, or fill a gap before optimizing. Pairs with [runtime-perf](../runtime-perf/SKILL.md) — author the benchmark here, measure with that.
---

# Benchmark Coverage

A new benchmark must (a) reliably exercise the path it claims, (b) take 1–5s on a dev machine in Release mode, (c) produce deterministic output, and (d) ship as a JIT-correctness regression test.

## Quick start — add a benchmark

1. **Pick a target path.** Name the opcode, IC kind, or JIT helper you want to exercise (e.g. `GET_FIELD_CACHED` MONO probe, `inline_polymorphic` POLY-4 dispatch, `OSR` from a tight `for` loop).
2. **Write the `.mt`** under `mType/tests/testFiles/benchmarks/<name>_hot.mt` (suffix `_hot` for JIT-hot targets, omit for interpreter-only). Mirror an existing structure — see `field_read_hot.mt`, `inline_polymorphic.mt`, `arithmetic_tight_loop.mt`.
3. **Header comment is mandatory.** Cite the MYT ticket, name the path under test, and state the target wall time. Future debuggers grep on this.
4. **Size N for 1–5s.** Start with a guess, run `--benchmark=<file> --benchmark-iterations=1`, scale N until median lands in band.
5. **Capture `.expected`** with `--no-jit` (see below). This is the JIT-correctness oracle (MYT-259 contract).
6. **Register in `IntegrationTestSuite.cpp`** in the `MYT-259: BENCHMARK JIT-CORRECTNESS REGRESSION TESTS` block. One `addOutputVerificationTest("Benchmark: <name>", benchmarkPath + "<name>.mt");` line, kept alphabetical with neighbors.
7. **Verify both passes green:** `--tests` (JIT on) AND `--tests --no-jit`.

## Authoring rules

- **Deterministic output only.** Print a final accumulator/checksum. Never print wall-clock time, addresses, GC counts, or anything that varies run to run.
- **No `import` of optional libs** unless the benchmark explicitly tests that integration. Standard lib primitives + collections only.
- **Exercise one thing.** A benchmark named `field_read_hot` does field reads. A benchmark testing both field reads and dispatch is two benchmarks.
- **Avoid `print()` in the hot loop** — print once at the end. Console I/O dominates and masks the path you're trying to measure.
- **Hot-loop iteration counts are constants**, not user input. The bytecode compiler folds constants; varying N at runtime hides constant-folding wins.
- **Name conventions:** `<feature>_hot.mt` for JIT-hot targets, `<feature>_tight_loop.mt` for opcode microbench, `<feature>_chain.mt` for sequenced calls, `<feature>_alloc.mt` for allocation/GC pressure.

## Capturing `.expected`

```powershell
bin\mType\Release\x64\mType.exe --no-jit `
    mType\tests\testFiles\benchmarks\<name>.mt `
    > mType\tests\testFiles\benchmarks\<name>.expected
```

Then verify JIT-on produces byte-identical output:

```powershell
bin\mType\Release\x64\mType.exe `
    mType\tests\testFiles\benchmarks\<name>.mt | `
    Compare-Object (Get-Content mType\tests\testFiles\benchmarks\<name>.expected) -SyncWindow 0
```

If they diverge, **do not check in `.expected` until the JIT bug is fixed or filed.** Keep the canary failing per [feedback_keep_failing_canary_tests](../../memory/feedback_keep_failing_canary_tests.md) — link to a MYT ticket.

## Coverage gap audit

Run when planning a perf push or before optimizing a subsystem.

### Step 1 — enumerate targets

Three coverage axes:

- **Opcodes** — `mType/vm/bytecode/OpCode.hpp`. Each opcode should have at least one benchmark that emits it in a tight loop. Grep the existing benchmark dir for the opcode name as a sanity hint.
- **JIT helpers** — `mType/vm/jit/JitHelpers_*.cpp`. Each `jit_*` helper signature should have a benchmark that triggers it. Use `--profile=full --profile-output=json` on each existing benchmark to capture which helpers fire, then diff against the helper inventory.
- **IC/dispatch paths** — MONO, POLY (2/3/4 entries), MEGA, deopt-and-recompile. Each path needs at least one canary.

### Step 2 — identify gaps

For each subsystem you're about to touch, list:
- Benchmarks that exercise it (grep `tests/testFiles/benchmarks/` for the opcode/helper name and read headers).
- Paths inside that subsystem with **no** benchmark hit (use `--jit-stats` deltas across the suite — zero-hit helpers stand out).
- Adjacent paths that should be canaries (e.g. if optimizing POLY-4, also add POLY-3 and MEGA so a future bug doesn't slip through).

### Step 3 — fill gaps before measuring perf

Bad order: optimize → measure → realize the benchmark doesn't actually exercise your path → re-do.
Good order: identify gap → author benchmark → capture `.expected` → register → only then start the perf work in [runtime-perf](../runtime-perf/SKILL.md).

## Common pitfalls

- **Wall-clock printed.** Run was probably copy-pasted from a one-off debugging script. Strip the `time` print before checking in.
- **Output varies across runs.** Usually GC-printing, identity hashes, or HashMap iteration order. Sort or sum to canonicalize.
- **N too small** (sub-100ms). JIT tiering thresholds may not fire; the benchmark measures startup, not steady-state.
- **N too big** (>10s). CI cost explodes; pick N for ~2s and let `--benchmark-iterations` average noise.
- **Forgot to register in `IntegrationTestSuite.cpp`.** The `.mt` runs under `--benchmark` but the JIT-correctness regression doesn't. Easy miss; always grep the suite for your new name before committing.
- **Used a stdlib not yet generic-safe** ([project_foreach_loses_nested_generic_type](../../memory/project_foreach_loses_nested_generic_type.md)). Stick to documented working patterns.
