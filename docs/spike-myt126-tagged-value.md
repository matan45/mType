# MYT-126 SPIKE — Shrinking `Value` from a 15-variant to a 16-byte tagged union

**Status:** Scaffolding landed; benchmark + full-path migration is follow-up work.
**Epic:** MYT-118 (Interpreter Performance Optimization Roadmap)
**Depends on:** MYT-119 (benchmark harness — landed)

---

## 1. Goal

Evaluate whether shrinking `value::Value` from the current `std::variant` layout (~56 bytes, 15 alternatives) to a 16-byte tagged union is worth the full-migration cost. Produce a go/no-go recommendation backed by measured arithmetic + object benchmarks.

## 2. Acceptance criteria (from Jira)

1. Prototype branch with a new `Value` layout behind a feature flag.
2. At least arithmetic and object benchmarks run on both paths.
3. Report with before/after numbers + effort estimate for full migration committed as a markdown doc (this file).
4. Go/no-go decision recorded on MYT-118.

## 3. Baseline — what the current `Value` costs

- `mType/value/ValueType.hpp:50` — `std::variant` of 15 alternatives (8 `shared_ptr<T>`, 5 inline primitives, 2 sentinels).
- **`sizeof(Value) == 56 bytes`** on MSVC x64 (8-byte aligned; tail-byte discriminant). `vm/jit/JitCompiler_EmitHelpers.cpp:423` static_asserts `sizeof(Value) % 8 == 0`.
- Executor code under `vm/runtime/executors/` makes **~388 variant accessor calls** (`std::get<>`: 178, `holds_alternative<>`: 210) on hot paths. Repo-wide accessor count is **~1069** — 2.7× the executor subset. This is a material finding: the full migration touches far more of the codebase than the original Jira sizing suggests.
- Stack is `std::vector<value::Value>` (`vm/runtime/stack/StackManager.hpp`) — every push/pop moves ~56 bytes.
- Arithmetic hot path (`ArithmeticExecutor.cpp:108-122`, `ADD_INT`): 2× `holds_alternative<int64_t>` + 2× `std::get<int64_t>` per instruction, before arithmetic + push.
- JIT tier (`vm/jit/`) already uses narrower per-slot representations in registers (`JitEmissionState::SlotType`) and only reads full `Value` through helper ABIs — **helper ABIs must be preserved**.

## 4. Design recommendation (chosen)

**16-byte POD struct with 1-byte tag + 8-byte payload + 7 bytes tail padding.** Tag is the existing `ValueType` enum. Payload is a union of `int64_t`, `double`, `bool`, `RefCounted*`, plus a spare handle slot for `InternedString` (via pooled id encoding in follow-up work).

```
Value {
    uint64_t payload;   // 8 bytes
    uint8_t  tag;       // 1 byte
    uint8_t  pad[7];    // 7 bytes — reserved for GC flags
}
sizeof == 16, alignof == 8
```

### 4.1 Rejected alternatives

- **NaN-boxing (8-byte Value).** Rejected: MSVC + v145 + 15 tags + non-IEEE double semantics + no existing bit-twiddling infrastructure is too much SPIKE-level risk. Revisit as a separate follow-up SPIKE if the 16-byte layout proves insufficient.
- **3-bit pointer-tag on aligned pointers.** Rejected: once we're at 16 bytes, low-bit tagging saves nothing and forces alignment guarantees we don't control (`PromiseValue` holds a `std::mutex`, for instance).

### 4.2 Collapsing the 8 `shared_ptr<T>` alternatives

Replace `std::shared_ptr<T>` with an intrusive **hand-rolled `IntrusivePtr<T>` + `RefCounted` base** (`mType/value/IntrusivePtr.hpp` — landed in this SPIKE). Non-atomic refcount; the VM loop is single-threaded. Retaining `std::shared_ptr` and tagging its control-block pointer is a correctness minefield (`make_shared` incompatibility, deleter rewrap, `enable_shared_from_this` breaks).

**Per-type migration cost (heap types targeted by shared_ptr in the variant):**

| Type | Flag-on migration |
|---|---|
| `ValueObject`, `NativeArray`, `FlatMultiArray`, `SparseMultiArray`, `FlatMultiObjectArray`, `BytecodeLambda`, `PromiseValue` | Trivial — add `: public RefCounted`. |
| `ObjectInstance` | **Invasive.** Uses `enable_shared_from_this` with 9 call sites. Requires an `intrusiveSelf()` shim routed to `shared_from_this()` under flag-off and `IntrusivePtr(this)` under flag-on. |

**SPIKE scope**: only `ObjectInstance` + `BytecodeLambda` get the full intrusive conversion (minimum for `object_alloc.mt`). The other six heap types are stubbed under flag-on via a thin adapter — they compile but are not benchmarked.

## 5. Feature flag strategy

`MTYPE_TAGGED_VALUE` preprocessor define, following the `MTYPE_SIMD_ENABLED` precedent at `premake5.lua:18`. A single `Value` class with two `#ifdef`-gated bodies keeps the public name stable and avoids accidental API divergence.

**Current state (scaffolding):** flag is **commented-out by default** in `premake5.lua` so flag-off builds are bit-for-bit identical to pre-SPIKE. Toggling on enables the tagged `Value` and **must** be paired with disabling JIT (benches already support `--no-jit`).

## 6. Accessor shim — bulk of the SPIKE migration

Call sites migrate from `std::holds_alternative<int64_t>(v)` / `std::get<int64_t>(v)` to free-function accessors `value::isInt(v)` / `value::asInt(v)` (`mType/value/ValueShim.hpp` — landed). Under flag-off the shim inlines to variant access. Under flag-on it reads tag/payload directly.

**Scope for the prototype**: migrate only the ~260 call sites on the two benchmark paths, not all ~1069. Cold subsystems (JIT helpers, JSON, reflection, debugger, natives) get `#ifndef MTYPE_TAGGED_VALUE` walls — they compile out under flag-on. **This is the SPIKE's honesty line.** Full migration is estimated below, not performed.

**Hot-path files to migrate (~260 sites)**:
`ArithmeticExecutor.cpp` (89), `ComparisonExecutor.cpp` (40), `ControlFlowExecutor.cpp` (1), `VariableExecutor.cpp` (2), `ObjectExecutor.cpp` (45), `ObjectInstanceHelper.cpp` (4), `InlineCacheExecutor.cpp` (11), `FunctionExecutor.cpp` (28), `VirtualMachineLoop.cpp` (6), `VirtualMachineHelpers.cpp` (37), `StackManager.*`.

## 7. Benchmark protocol

Three paths, `--benchmark-output=json`, 5 iterations each, record median + min.

| Path | Build | JIT | Purpose |
|---|---|---|---|
| A | flag-off | on | Current production baseline |
| B | flag-off | `--no-jit` | **Honest baseline vs. C** (apples-to-apples, JIT off for both) |
| C | flag-on  | `--no-jit` | Tagged-Value prototype |

Primary scripts: `mType/tests/testFiles/benchmarks/arithmetic_tight_loop.mt` (arithmetic hot path; ~236M instructions) and `mType/tests/testFiles/benchmarks/object_alloc.mt` (object allocation; ~2M calls). Secondary if cheap: `method_dispatch.mt`, `array_multi_get.mt`.

Also capture: `sizeof(Value)` at startup, peak RSS via `GetProcessMemoryInfo`. **The memory win is the actual headline** — a 3.5× shrink on the operand stack is expected to dwarf any per-instruction cycle difference on hot loops.

## 8. Results — to fill in after benchmark runs

> **Instructions for the runner**: toggle `MTYPE_TAGGED_VALUE` in `premake5.lua:18`, re-run `runPremake.bat`, rebuild `Interpreter.sln` Release, rerun the benches, paste results here.

### 8.1 Size & memory

| Metric | Flag-off | Flag-on | Δ |
|---|---|---|---|
| `sizeof(Value)` | 56 B | 16 B | −71% |
| Peak RSS (`arithmetic_tight_loop.mt`) | _TBD_ | _TBD_ | _TBD_ |
| Peak RSS (`object_alloc.mt`)          | _TBD_ | _TBD_ | _TBD_ |

### 8.2 Arithmetic — `arithmetic_tight_loop.mt`

| Path | median ms | min ms | Δ vs. B |
|---|---|---|---|
| A (flag-off + JIT)       | _TBD_ | _TBD_ | — |
| B (flag-off + --no-jit)  | _TBD_ | _TBD_ | baseline |
| C (flag-on  + --no-jit)  | _TBD_ | _TBD_ | _TBD_ |

### 8.3 Objects — `object_alloc.mt`

| Path | median ms | min ms | Δ vs. B |
|---|---|---|---|
| A | _TBD_ | _TBD_ | — |
| B | _TBD_ | _TBD_ | baseline |
| C | _TBD_ | _TBD_ | _TBD_ |

## 9. Full-migration effort estimate

Based on the actual 1069 accessor-call count (not the initial 388 executor-only estimate):

| Area | Work | Hours |
|---|---|---|
| Remaining call-site sweep (~800 sites) | mechanical `holds_alternative`/`get` → `isX`/`asX` rewrites + review | 40–70 |
| Intrusive conversion of 6 remaining heap types | `: public RefCounted` + minor ctor/dtor tidy | ~6 |
| JIT helper rewrite | preserve helper ABIs, swap internal unpacking | 20–30 |
| Test-suite fixes | suites that poke `Value` internals | ~15 |
| Debugger / JSON / reflection / natives | variant-aware subsystems | ~15 |
| **Total** | | **~100–140 h (~13–17 SP)** |

This is materially higher than MYT-126's 5-point sizing (the SPIKE itself). The go/no-go decision should weigh the measured speedup/memory-win against the 13–17 SP commitment, not against the SPIKE's 5 points.

## 10. Decision rule

- **GO** if arithmetic ≥ **15%** faster (C vs. B) **OR** RSS drops ≥ **25%**.
- **NO-GO** if arithmetic regresses > **5%** (C vs. B) *and* RSS savings are <10%.
- Otherwise **CONDITIONAL** — schedule a follow-up SPIKE on NaN-boxing before committing.

> **Recommendation to record on MYT-118**: _pending bench results in §8._

## 11. Risks & gaps

- **`enable_shared_from_this` on `ObjectInstance`** — 9 call sites. Mitigated by `intrusiveSelf()` shim; ensure no path escapes the shim (code review needed during full migration).
- **1069 accessor sites repo-wide** — 2.7× the original estimate. The `#ifndef MTYPE_TAGGED_VALUE` wall around cold subsystems is a compile-time honesty device, not a testing strategy: flag-on builds will not exercise those paths.
- **MSVC v145 + non-trivial union member destruction** — tagged `Value`'s destructor dispatches on tag; verify with `_CrtDumpMemoryLeaks` on Debug under flag-on.
- **`std::string` & `InternedString` boxing** — not benchmarked. String-heavy workloads may regress under flag-on; disclosed, not measured.
- **`PromiseValue` holds a `std::mutex`** — not trivially relocatable, but intrusive refcount doesn't require relocation.
- **No `std::visit` usage** in the Value path — the free-function shim approach is viable everywhere.

## 12. What landed in the scaffolding commit (this SPIKE)

- `premake5.lua:18-21` — `MTYPE_TAGGED_VALUE` define, commented-out.
- `mType/value/IntrusivePtr.hpp` — `RefCounted` base + `IntrusivePtr<T>` + `makeIntrusive<T>(...)`.
- `mType/value/ValueType.hpp` — dual-body: `std::variant` under flag-off (unchanged) + 16-byte tagged `class Value` under flag-on with static_assert on size and alignment.
- `mType/value/ValueShim.hpp` — free-function accessors (`isInt`/`asInt`/…); flag-off delegates to variant, flag-on reads tag/payload.
- This report.

**Flag-off builds are byte-for-byte unchanged** — all new code sits under `#ifdef MTYPE_TAGGED_VALUE` or in standalone headers not included by default.

## 13. Follow-up work (not in this SPIKE's scope)

Prerequisites for running the benchmarks and populating §8:

1. Migrate `ObjectInstance` + `BytecodeLambda` to inherit `RefCounted` (guarded by `#ifdef MTYPE_TAGGED_VALUE`), with `intrusiveSelf()` shim.
2. Migrate the ~260 hot-path accessor sites in §6 to `ValueShim` functions.
3. Wall off JIT / JSON / reflection / debugger / natives under `#ifndef MTYPE_TAGGED_VALUE`.
4. Stub the 6 non-benchmarked heap types (`ValueObject`, four array types, `PromiseValue`) behind a thin `RefCounted` adapter under flag-on.
5. Run the benches per §7, populate §8, record the go/no-go decision on MYT-118 per §10.
