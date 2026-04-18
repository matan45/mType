# Phase F: Function Inlining — Design Doc

Status: DESIGN. Tracked as **MYT-162**. Prerequisite (MYT-161) shipped — methods now JIT-compile, IC has cached `jitEntry` pointer, `emitDeoptToInterpreter` helper is latent on branch.

## 1. Why

`docs/bench-baseline.md` 2026-04-18 MYT-161 c section documents the cost model:

- JIT dispatch path per call: ~200–300 cycles setup (`CallFrame` push, `JitContext{}` zero-init with 16 `value::Value` slots, arg unbox, return box) + ~20 cycles native body for a 3-op method
- Interpreter dispatch path per call: ~300–400 cycles (save state + ~5 ops × ~50 cycles dispatch + cleanup)

For tiny polymorphic methods (`method_dispatch.mt`'s `A/B/C::compute(x): return x * binop n`), per-call setup dominates the body. **No amount of dispatch optimization can move the needle** — only eliminating the call frame entirely, via inlining, can.

## 2. Goal / Non-goals

**Goal:** At JIT emit time, speculatively inline the body of a small monomorphic (and, in a later phase, polymorphic-with-guards) callee directly into the caller's native code stream, with shape-guard + deopt fallback.

**Target benchmarks (measurable impact expected):**

- `method_dispatch.mt` 1298ms → ~700ms (-46%) when all three polymorphic branches inline with guards (Phase F-c). MONO-only (F-a) gets partial credit.
- `for_each_loop.mt` additional ~5-10% on top of MYT-161's -11% — iterator `hasNext`/`next`/`close` and `Int::getValue` are all MONO call sites with small bodies.
- `recursive.mt` marginal — `gcd` is the only non-self-recursive leaf, and it's only ~5 ops. fib/ack are self-recursive, not inlineable.

**Non-goals for this ticket:**

- Generic methods (`<T>` specialization) — skip, complexity mismatch.
- Async methods (bodies containing `AWAIT`) — skip.
- Super calls inside inlined methods — skip; too tangled with method-resolution semantics for v1.
- Escape analysis / allocation sinking (Phase C / MYT-118) — separate work that compounds with F but independent.
- ObjectInstance shape redesign (Phase D) — separate.

## 3. Prerequisites (already on MYT-161 branch)

1. **Methods JIT-compile** — MYT-161 `PROFILE_ENTER` emission in `MethodCompilerHelper::compileMethod`. Without this, callees don't make it into `jitCodeCache`, IC's cached `jitEntry` stays null, speculative inlining has nothing to speculate on.
2. **IC populates cached `jitEntry`** — MYT-161 `tryDirectJitMethodDispatch`. Gives the inliner a pre-resolved function pointer (even though we're inlining the *bytecode*, not the compiled code — the `jitEntry` tells us the callee is stable enough to speculate on).
3. **`emitDeoptToInterpreter` helper** — MYT-132-pre. Flushes current locals to `osrOutputLocals` and invokes `jit_osr_deoptimize` with a bytecode offset to resume from. This is the shape-guard-mismatch fallback.

## 4. Architectural choice: JIT-time speculative inlining

Two axes:

- **When:** bytecode-compile time (static, cacheable in `.mtc`) vs JIT-emit time (speculative, uses runtime IC feedback).
- **What:** the callee's bytecode ops (cross-function transform on bytecode IR) vs the callee's compiled machine code (copy-paste compiled bytes).

**Chosen: JIT-emit time, bytecode-level expansion.**

- Bytecode-compile time can't handle polymorphic sites (no IC feedback). `method_dispatch.mt` is polymorphic — that's the primary target. Must be runtime.
- Bytecode-level expansion reuses the existing per-opcode emit functions (`emitArithmeticOps`, `emitObjectOps`, etc.). Machine-code copy-paste requires parsing and relocating asmjit output — substantially harder.

The flow at a CALL_METHOD site:

```
Caller JIT emit encounters CALL_METHOD at offset ip_call:
  ├─ Check IC at ip_call
  │   ├─ MONO + eligible callee: inline
  │   └─ POLY / MEGA / not-eligible: fall through to current jit_call_method_ic path
  │
  ├─ (MONO case) Emit:
  │   ├─ Shape guard on receiver's classDef
  │   ├─ Allocate fresh local slots in caller's frame for callee's locals
  │   ├─ Load callee locals[0..N] from callArgs[0..N]
  │   ├─ Recursively run emitCodegenLoop over callee's [startOffset, endOffset) range
  │   │   with an InlineContext pushed on JitEmissionState.inlineStack
  │   ├─ Inline-end label: result already on caller's operand stack
  │   └─ Deopt label: emitDeoptToInterpreter(ip_call)
  │
  └─ Continue emitting caller's next instruction
```

## 5. Data structures

### 5.1 Eligibility check

```cpp
// vm/optimization/InlineAnalysis.{hpp,cpp} — new
namespace vm::optimization
{
    struct InlineEligibility
    {
        enum class Decision
        {
            INLINE,             // All checks passed
            CALLEE_TOO_BIG,     // instructionCount > limit
            HAS_TRY_CATCH,      // Contains TRY_BEGIN
            HAS_ASYNC,          // Contains AWAIT / CREATE_PROMISE
            HAS_UPVALUES,       // Contains LOAD_UPVALUE / STORE_UPVALUE
            HAS_SUPER_CALL,     // Contains SUPER_INVOKE / SUPER_CONSTRUCTOR
            SELF_RECURSIVE,     // Callee qualifiedName matches caller being compiled
            DEPTH_EXCEEDED,     // inlineStack depth >= limit
            IC_NOT_MONOMORPHIC, // IC state != MONO (for v1; v2 handles POLY)
            UNKNOWN_SHAPE,      // IC entry's shape or funcMeta is null
        };
        Decision decision;
    };

    InlineEligibility checkInlineEligibility(
        const bytecode::BytecodeProgram::FunctionMetadata& callee,
        const bytecode::BytecodeProgram& program,
        const vm::jit::ic::MethodInlineCache& ic,
        const std::string& currentCompilingFn,
        size_t currentInlineDepth);

    constexpr size_t INLINE_SIZE_LIMIT = 16;   // callee bytecode ops
    constexpr size_t INLINE_DEPTH_LIMIT = 2;   // nested inline depth
}
```

### 5.2 JitEmissionState additions

```cpp
// vm/jit/JitEmissionState.hpp — add to struct JitEmissionState
struct InlineFrame
{
    const bytecode::BytecodeProgram::FunctionMetadata* calleeMeta;
    size_t localsBaseSlot;        // Slot offset in caller's locals[] where callee locals start
    asmjit::Label inlineEndLabel; // Where RETURN_VALUE jumps to
    size_t returnResultSlot;      // Stack slot where result lands for caller
    std::unordered_map<size_t, asmjit::Label> localJumpLabels; // Callee IP → label
};

std::vector<InlineFrame> inlineStack;
```

### 5.3 Pre-scan for local-slot reservation

The asmjit frame is allocated once in `setupCompilationFrame` with a fixed `localCount`. For inlining, we need extra slots for potential inlinees. Pre-scan the function's bytecode before frame allocation:

```cpp
// JitCompiler_Core.cpp — enhance setupCompilationFrame
size_t computeInlineLocalsBudget(const bytecode::BytecodeProgram& program,
                                  const FunctionMetadata& funcMeta,
                                  const ic::InlineCacheTable& icTable);
// Walks funcMeta's instructions, for each CALL_METHOD checks IC, sums potential
// callee localCount. Returns max simultaneous locals needed. Conservative — may
// over-allocate but never under-allocates.
```

Frame size becomes `funcMeta.localCount + computeInlineLocalsBudget(...)`.

## 6. Emission algorithm

### 6.1 At CALL_METHOD emit site

Pseudo-code for `tryEmitInlinedMethodCall(s, instr)` (new function in `JitCompiler_Objects.cpp`, called from `emitCallMethodOp` before current fallback):

```
1. ic = s.icTable->getMethodIC(s.currentIP)
2. If ic.state != MONOMORPHIC, return false
3. entry = ic.entries[0]
4. If entry.shape == nullptr || entry.funcMetadata == nullptr, return false
5. callee = entry.funcMetadata
6. eligibility = checkInlineEligibility(callee, program, ic, currentFn, s.inlineStack.size())
7. If eligibility != INLINE, return false

8. Emit shape guard:
   - Extract receiver classDef ptr from ctx->callArgs[0]
   - cmp classDef, <immediate: entry.shape>
   - jne deoptLabel

9. Allocate inline frame:
   - localsBaseSlot = nextAvailableInlineSlot (tracked in JitEmissionState)
   - inlineEndLabel = cc.new_label()
   - returnResultSlot = s.stackDepth (where caller expects result)

10. Load callee's parameters from callArgs into inlined locals:
    - for i in 0..callee.parameterCount:
        copy callArgs[i] → localsBase[(localsBaseSlot + i) * localStride]

11. Zero-init remaining callee locals:
    - for i in parameterCount..localCount:
        zero localsBase[(localsBaseSlot + i) * localStride]

12. Push InlineFrame onto s.inlineStack

13. Emit callee's ops:
    - Collect labels for callee's internal jumps (pre-scan)
    - Run a modified emitCodegenLoop over [callee.startOffset, callee.startOffset + callee.instructionCount)
    - During emission:
        - LOAD_LOCAL/STORE_LOCAL: operand += localsBaseSlot  (remapped)
        - JUMP/JUMP_*: target is callee's internal label, NOT caller's
        - RETURN_VALUE: handled specially — push onto caller's stack at returnResultSlot, jump to inlineEndLabel
        - RETURN: same, with monostate result
        - PROFILE_ENTER: skip (no-op anyway, but skip for clarity)

14. Bind inlineEndLabel

15. Pop s.inlineStack

16. Caller's stackDepth now reflects the pushed result; continue with caller's next instruction.

17. Emit deopt handler (after inline body):
    - deoptLabel:
    - emitDeoptToInterpreter(s, s.currentIP) — bails to interpreter at the original CALL_METHOD offset
```

### 6.2 RETURN_VALUE substitution

When emitting RETURN_VALUE inside an inline frame:

```
// Normal RETURN_VALUE at top level:
// - Pop operand stack top → ctx->returnValue
// - ctx->hasReturnValue = true
// - Jump to function epilogue

// Inline RETURN_VALUE:
// - Operand stack top is the result
// - Move it (with appropriate boxing) to returnResultSlot in caller's stack
// - Jump to inlineEndLabel
// - Leave caller's operand stack depth set as if the call pushed a result
```

A dedicated `emitInlineReturn(s, resultType)` helper handles boxing: if caller expected boxed return (via `s.usesBoxedTypes`), box the raw result; otherwise pass through.

### 6.3 Internal jump handling

Callee's JUMP / JUMP_IF_FALSE / etc. use operands that are bytecode offsets. These must become asmjit labels within the inlined region. Pre-scan the callee's instructions:

```
calleeLabels: unordered_map<size_t, asmjit::Label>
for ip in [callee.startOffset, callee.startOffset + callee.instructionCount):
    if instr.opcode is jump-like:
        target = instr.operands[0]
        if target not in calleeLabels:
            calleeLabels[target] = cc.new_label()
```

During emission: when binding a label at offset `ip`, check `calleeLabels[ip]`. When emitting a jump, use `calleeLabels[target]` as the asmjit label.

## 7. Polymorphic inlining (Phase F-c)

For POLY sites (multiple shapes), emit chained guards:

```
cmp classDef, <shape 0>
je inline_0
cmp classDef, <shape 1>
je inline_1
...
jmp fallback_helper_call

inline_0:
  <inlined body for shape 0>
  jmp inline_end

inline_1:
  <inlined body for shape 1>
  jmp inline_end

fallback_helper_call:
  call jit_call_method_ic  ; existing path for MEGA / miss

inline_end:
```

Eligibility for POLY inlining: each shape's callee must independently pass the size/complexity checks, AND their combined inlined size must not blow up code cache (budget: total inlined instructions ≤ INLINE_SIZE_LIMIT × IC_MAX_POLYMORPHIC_ENTRIES = 64 ops).

## 8. Deopt strategy

mType does **not** allow runtime class redefinition (confirmed). This collapses the global-invalidation branch — only per-site deopt matters.

**Shape-guard mismatch (single call, runtime):**

Shape guard compares `receiver.classDef` against the cached shape immediate embedded at emit time. On mismatch, jump to deopt label which calls `emitDeoptToInterpreter(ip_call)`. The interpreter resumes at the CALL_METHOD instruction and runs the slow path. The current JIT'd function exits. Next call to the JIT'd caller re-enters normally — no recompilation, just deopt for this single in-flight call.

Shape mismatches still happen without class redefinition: an IC can transition MONO → POLY at runtime when a new receiver type hits the site (e.g., first call sees class A, later calls see class B). The F-a emission speculates on the MONO-state cache entry observed at JIT time. If the site later becomes POLY, subsequent in-flight calls hit the shape-guard miss and deopt to the interpreter, which goes through the normal `jit_call_method_ic` path and populates the cache as POLY. The JIT'd caller code becomes stale for that site but self-corrects via repeated deopt. A follow-up recompile (triggered by future hot-code tiering) would re-emit with current IC state — for v1, we accept the ongoing deopt cost on a POLY-transitioned site until recompile.

**No global-invalidation branch needed in v1.** `InlineCacheTable::invalidateAll` remains a stub. If runtime class redefinition is added later, revisit — see MYT-166 (deferred).

## 9. Local-slot pre-scan

`setupCompilationFrame` currently allocates `localCount * localStride` bytes for locals. With inlining, it must allocate more. Algorithm:

```
computeInlineLocalsBudget(program, funcMeta, icTable):
    maxSimultaneousLocals = 0
    pretend-emit walk of funcMeta's instructions:
        on CALL_METHOD at ip:
            ic = icTable.getMethodIC(ip)
            if ic.MONO and callee is eligible:
                calleeLocals = callee.localCount
                # Recursive: if callee itself inlines something, account for that
                nestedBudget = computeInlineLocalsBudget(program, callee, icTable)
                currentBudget = calleeLocals + nestedBudget
                maxSimultaneousLocals = max(maxSimultaneousLocals, currentBudget)
    return maxSimultaneousLocals
```

Conservative: reserves max possible inline stack even if some sites don't actually inline at runtime (e.g., ICs are in a different state). Unused slots are cheap — just stack space.

Gotcha: recursive inlining analysis must bail at INLINE_DEPTH_LIMIT to avoid infinite recursion in the budget calculator.

## 10. Critical files to modify

| File | Role |
|------|------|
| `mType/vm/optimization/InlineAnalysis.{cpp,hpp}` *(new)* | Eligibility checks, const limits |
| `mType/vm/jit/JitEmissionState.hpp` | Add `InlineFrame` struct, `inlineStack` vector |
| `mType/vm/jit/JitCompiler_Objects.cpp` | `tryEmitInlinedMethodCall` in `emitCallMethodOp` |
| `mType/vm/jit/JitCompiler_Core.cpp` | Pre-scan for locals budget, remapping in `emitCodegenLoop` |
| `mType/vm/jit/JitCompiler_ControlFlow.cpp` | Inline-aware RETURN_VALUE, internal jump remapping |
| `mType/vm/jit/JitCompiler_EmitHelpers.cpp` *(or new)* | `emitInlineReturn`, `emitInlineShapeGuard`, `emitInlineLocalCopy` |
| `mType/vm/jit/ic/InlineCacheTable.cpp` | Wire `invalidateAll` into class-registry mutation points |
| `mType/vm/jit/JitCodeCache.cpp` | Accept global invalidate on IC-wide invalidation |
| `mType/vm/runtime/VirtualMachineJit.cpp` | Hook IC invalidation from class-registry writes |

## 11. Incremental ship plan

**F-a (2-3 days): Basic MONO inlining, no internal jumps in callee.**
- InlineAnalysis + eligibility (size≤16, no jumps in callee, no try/catch/async/upvalues, not self-recursive, depth<2)
- JitEmissionState::inlineStack
- `tryEmitInlinedMethodCall` with shape guard + copy args + emit straight-line callee + emit RETURN_VALUE substitution
- Local-slot pre-scan for budget
- Deopt via existing `emitDeoptToInterpreter`
- **Smoke test:** `A::compute(x): return x * 2` inlines correctly; `method_dispatch` with monomorphic test variant drops as expected.
- **Sanity test:** all existing benchmarks and `mtype-tests` suite pass.
- **Ship & bench.**

**F-b (2-3 days): Callees with internal jumps + nested CALL_METHOD.**
- Pre-scan callee for internal jump targets, create labels per callee-IP
- Remap jump operands to labels during emission
- Allow inlined callee to itself inline (depth > 1 up to INLINE_DEPTH_LIMIT)
- **Smoke test:** inline an iterator fast-path with internal `if`, verify `for_each_loop` drops further.

**F-c (1-2 days): POLY inlining with chained shape guards.**
- Extend eligibility to POLY (all entries must be inlineable)
- Emit chained guards + specialized bodies
- Fallback: chain ends in `call jit_call_method_ic`
- **Smoke test:** `method_dispatch.mt` drops ≥30% (from ~1298 to ~900ms expected).

**F-d (1 day): IC invalidation → JIT cache invalidation.**
- Wire `InlineCacheTable::invalidateAll` to `JitCodeCache::clear`
- Identify class-registry mutation hook points
- **Smoke test:** reflection-based class redefinition regression tests pass.

## 12. Testing strategy

### 12.1 Unit tests

New file `mType/tests/testFiles/integration/inlining/`:
- `inline_basic.mt` — simple method, verify output unchanged
- `inline_arithmetic.mt` — `x.add(y)` inlined, bench-style hot loop
- `inline_poly.mt` — `A/B/C::compute` pattern (mirror of `method_dispatch.mt` but smaller)
- `inline_deopt.mt` — shape mismatch at runtime (e.g., adding a new subclass mid-execution, if the language allows); verify deopt + correct result
- `inline_recursive.mt` — self-recursion not inlined (regression guard)

### 12.2 Regression

Full `mType.exe` test suite. Focus areas:
- reflection (any class-redefinition path → must invalidate JIT)
- generic methods (must not be inlined in v1)
- exception handling (TRY_BEGIN callees must not inline)
- async methods (AWAIT callees must not inline)

### 12.3 Bench gates

On each ship:
- `method_dispatch.mt` — target -30%+ (POLY inlining), -10% MONO-only
- `for_each_loop.mt` — target additional -5-10%
- **Regression tripwire:** any benchmark regressing >3% with no offsetting win → revert on branch (per MYT-132 precedent)

## 13. Open questions to resolve during implementation

1. **`sharedptr` receiver extraction** — reading `callArgs[0].instance->classDef` in asm requires understanding `std::shared_ptr<ObjectInstance>` layout under MSVC. Option: expose a pure-C helper `jit_extract_classdef(Value*)` that returns the raw classDef pointer, call it from emitted code. Trades one helper call for correctness.

2. **Immediates vs IC-entry reads for shape comparison** — embedding `entry.shape` as an asm immediate is fast (no load) but requires recompilation if the IC entry changes. Alternative: load from the IC entry slot at runtime. Tradeoff: 1 extra load per guard vs exposure to IC rehash. Recommend: immediate, with IC invalidation triggering recompile.

3. **Return-value boxing / unboxing mismatch** — caller JIT may be in "boxed mode" (`s.usesBoxedTypes = true`) while callee expects unboxed ints. Resolve during arg load + return handling via the existing `emitEnsureUnboxed` / `emitBox` helpers, but need care to re-box for caller if boxed mode.

4. **Exception propagation** — if a non-try-catch callee throws (e.g., null pointer), the exception must unwind through the inlined frame as if it were a real call. Current mechanism: `ctx->pendingException` is checked at helper-call boundaries. Inlined bodies don't have those boundaries. Need to add periodic pending-exception checks, or restrict inlining to demonstrably non-throwing callees (no GET_FIELD / ARRAY_GET / etc. without INSTR_FLAG_NONNULL_RECEIVER). Lean toward restriction for v1.

5. **Debug info** — inlined ops won't have their own call-frame in stack traces. Decide: add source-location tracking to inlined emissions (complex) or document the limitation (simple). Recommend: document for v1.

## 14. Design questions — answered

1. **Runtime class redefinition:** *Not supported.* Simplifies F-d to paranoia scaffolding only; deferred indefinitely (MYT-166 closed). Shape-guard deopt is still required for MONO→POLY IC transitions — that's independent of class-redefinition and must land in F-a.

2. **F-a acceptance target:** ship against a *dedicated MONO test benchmark* (single-class `compute`-style hot loop). `method_dispatch.mt` stays flat until F-c; don't block F-a on the headline benchmark. Sequential F-a → F-b → F-c; each ships and benches independently.

## 15. Receiver-type restriction for v1

mType's `CALL_METHOD` receiver slot can hold one of two heap-wrapped kinds:

- `std::shared_ptr<runtimeTypes::klass::ObjectInstance>` — reference classes
- `std::shared_ptr<value::ValueObject>` — opt-in value types (produced by `OBJECT_TO_VALUE`, copy-on-write)

**v1 restricts inlining to `ObjectInstance` receivers.** The shape-guard extracts a `ClassDefinition*` from the receiver, which requires the ObjectInstance layout. Current Phase A fast path (`JitHelpers_Objects.cpp:101`) already guards this — ValueObject callers fall through to the generic `jit_call_method` helper, which wraps ValueObject in a temporary ObjectInstance for dispatch. Inlining mirrors that: `InlineEligibility` returns `UNKNOWN_SHAPE` (or a new `VALUE_OBJECT_RECEIVER` decision) when the IC's cached `shape` belongs to a ValueObject-typed call site, falling through to the existing helper path.

Detection at JIT emit time: the IC entry only stores `const ClassDefinition*`, not the receiver kind. Two options:
- Track receiver kind in the IC entry (new `MethodICEntry` field: `bool receiverIsValueObject = false`). Requires updating IC populate in `jit_call_method_ic`. Lightweight.
- Query the `ClassDefinition` for a `"is value class"` flag if one exists. Check existing `ClassDefinition` API.

Recommend option 1 for clarity. ~4 LOC addition.
