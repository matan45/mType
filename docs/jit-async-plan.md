# JIT support for async/await — compile-with-deopt

## Context

After the inline-resolved-Promise<Int> change, the post-fusion async benchmarks
sit at ~1.15 µs/await on `async_await_tight_loop`. That's still ~14× the
~80 ns/call cost of `function_call_hot.mt` (sync, JIT-inlined), and the gap
is **purely JIT vs interpreter dispatch**: `JitCompiler::canCompile`
(JitCompiler_Core.cpp:18) hard-rejects any function with
`meta.isAsync == true`, so async function bodies never enter the JIT.

In our benchmarks every AWAIT lands on the synchronous-completion fast path
(VirtualMachineAsync.cpp:36-40). In real workloads this is also the
dominant case for cooperative-multitasking patterns. We can compile async
function bodies **optimistically assuming sync-completion at every AWAIT**
and bail out (deopt) to the interpreter only on the rare pending-promise
path — the same pattern V8, JSC, and the .NET CLR use.

Goal: per-await dispatch cost on the sync path drops from ~1.15 µs toward
~80 ns. Pending promises retain full event-loop semantics by deopting into
the existing interpreter `executeAwait`. Expected wall-time on
`async_await_tight_loop`: ~150-250 ms (down from 1145 ms today).

## Approach

Compile async function bodies through the existing JIT pipeline, with two
new opcodes (`AWAIT`, `CREATE_PROMISE`) supported via a sync-fast-path emit
strategy. On pending-promise the JIT-emitted code throws an
`OSRDeoptException` carrying the AWAIT's bytecode IP, which the existing
deopt machinery (DeoptimizationHandler.cpp) restores into the interpreter
frame at that exact IP. The interpreter then re-runs AWAIT through its
normal suspend path.

This is the **smallest change that gets the win** — no continuation-passing
rewrite, no new state-machine transformation. The fused
`OBJECT_TO_VALUE_CREATE_PROMISE` and `CREATE_PROMISE_RETURN_VALUE` opcodes
also need emit cases, but they collapse trivially to the same fast-path
shape as standalone `CREATE_PROMISE`.

## Critical reused infrastructure

| Piece | File:line | What it gives us |
|---|---|---|
| `OSRDeoptException` | `vm/jit/guards/DeoptimizationHandler.hpp:14-18` | Exception carrying the bytecode IP to resume at |
| `jit_osr_deoptimize` | `vm/jit/JitHelpers_OSR.cpp:24-27` | The throw site — we'll call it from AWAIT's slow path |
| `DeoptimizationHandler::handleDeopt` | `vm/jit/guards/DeoptimizationHandler.cpp:5-31` | Restores interpreter locals + sets IP. Already works at *any* bytecode offset, not just loop boundaries |
| `jit_osr_write_local` | `vm/jit/JitHelpers_OSR.cpp:8-14` | How JIT materializes locals into `ctx->osrOutputLocals` for deopt restoration |
| StackModel `flushAllHints` | `vm/jit/JitCompiler_StackModel.cpp` | Spills register-cached operand-stack slots back to memory before bailout |
| Inline `PROMISE_INT` tag | `value/ValueType.hpp` | Already represents sync-resolved promise as a tag-check — perfect for the AWAIT fast-path guard |

## Files to modify

### 1. Lift the function-level gate

`mType/vm/jit/JitCompiler_Core.cpp:18`

```cpp
// Before
if (meta.isNative || meta.isAsync) return false;
// After
if (meta.isNative) return false;
```

The `isAsync` reject is the *only* place async functions are gated out of
JIT compilation today. Once the new opcodes are in `getSupportedOpcodes()`,
lifting this is sufficient to attempt compilation.

### 2. Extend the supported opcode set

`mType/vm/jit/JitCompiler_StackOps.cpp:14` (`getSupportedOpcodes`)

Add four entries to the static `supported` set:

```cpp
static_cast<uint8_t>(OpCode::CREATE_PROMISE),
static_cast<uint8_t>(OpCode::AWAIT),
static_cast<uint8_t>(OpCode::OBJECT_TO_VALUE_CREATE_PROMISE),
static_cast<uint8_t>(OpCode::CREATE_PROMISE_RETURN_VALUE),
```

`canCompile` then accepts async function bodies whose only async-related
opcodes are these four. Bodies using anything else (e.g. `PROMISE_RESOLVE`,
which currently has no emitter) still bail.

### 3. Add the four emit cases

New file: `mType/vm/jit/JitCompiler_Async.cpp` (mirrors the naming of
`JitCompiler_ControlFlow.cpp` etc.). Wire it into the dispatch chain at
`JitCompiler_Core.cpp:529-554` (`emitCodegenLoop`) by adding an
`emitAsyncOps(s, instr)` call alongside the existing
`emitCoreOps`/`emitArithmeticOps`/`emitControlFlowOps`/`emitObjectOps`.

Per opcode:

**`CREATE_PROMISE`** — emit:
- `flushAllHints(s)` — spill register-cached stack slots so the helper sees a coherent stack.
- Pop TOS via the StackModel (load Value tag + payload).
- If the slot's static type is INT (StackModel knows this in many cases):
  emit inline construction of `PROMISE_INT` Value: write tag byte = `PROMISE_INT`, copy payload through. No helper call.
- Else (slot is heap or unknown): emit `cc.invoke` to a new helper
  `jit_create_promise(JitContext*, const Value*)` that runs the
  interpreter logic from `VirtualMachineDispatch.cpp` `case CREATE_PROMISE`
  (fast-paths INT, heap-allocates `AsyncPromiseValue` otherwise).

**`OBJECT_TO_VALUE_CREATE_PROMISE`** — emit:
- `flushAllHints(s)`.
- Always go through a helper `jit_object_to_value_create_promise(JitContext*)`
  that runs the existing fused interpreter handler verbatim. This is the
  path that detects Int boxes and emits inline `PROMISE_INT` — keep the
  logic in C++, don't try to inline the
  `getPrimitiveTag()`/`getFieldValue("value")` dance in asmjit.

**`CREATE_PROMISE_RETURN_VALUE`** — emit:
- Same fast-path tag-check as `CREATE_PROMISE` to produce the inline form.
- Then jump to the function epilogue / emit the same return sequence as
  `RETURN_VALUE` (look at `JitCompiler_ControlFlow.cpp` for the existing
  `RETURN_VALUE` emitter and reuse it).

**`AWAIT`** — the load-bearing one. Emit:

```
flushAllHints(s)
load TOS Value tag into a register
cmp tag, PROMISE_INT
je  fast_path_int     ; inline-resolved form: extract payload_.i, push as INT
cmp tag, PROMISE
jne type_error_path   ; not a promise — throw RuntimeException via helper
; heap PROMISE form: check isFulfilled bit
load bridge ptr from payload_.ptr
load promise->state into reg
cmp state, FULFILLED
je  heap_resolved_path ; pull value from bridge, push, fall through
; pending or rejected — DEOPT
call jit_await_deopt(ctx, currentBytecodeIP)  ; throws OSRDeoptException
; (unreachable)

fast_path_int:
  read payload_.i, push Value(int64_t) with tag INT
  jmp done

heap_resolved_path:
  call helper jit_await_extract_resolved(ctx, promisePtr)
  push returned value
  jmp done

type_error_path:
  call jit_await_type_error(ctx)  ; throws RuntimeException via existing pendingException stash

done:
```

The `currentBytecodeIP` passed to `jit_await_deopt` is `s.currentIP` —
already tracked per-instruction in `JitCompiler_Core.cpp:535`
(`s.currentIP = ip`). The deopt handler will restore locals, set
`context.instructionPointer = ip - 1`, and the interpreter loop will
re-execute the same AWAIT — this time taking its existing suspend path.

### 4. New runtime helpers

New file: `mType/vm/jit/JitHelpers_Async.cpp`. Declare in
`JitHelpers.hpp`. All follow the existing `JitContext*`-first calling
convention.

```cpp
extern "C" {
    // Heap CREATE_PROMISE path. Mirrors VirtualMachineDispatch.cpp's
    // CREATE_PROMISE handler (the post-inline-resolved-Promise version).
    void jit_create_promise(JitContext* ctx, const value::Value* val);

    // Fused OBJECT_TO_VALUE_CREATE_PROMISE — runs the existing fused
    // handler from VirtualMachineDispatch.cpp verbatim.
    void jit_object_to_value_create_promise(JitContext* ctx);

    // AWAIT slow-path resolved heap promise: pulls the value out and
    // returns it on the JIT operand stack via ctx.
    void jit_await_extract_resolved(JitContext* ctx, value::PromiseValue* promise);

    // AWAIT pending/rejected — throws OSRDeoptException with the IP.
    // Tail-calls jit_osr_deoptimize.
    void jit_await_deopt(JitContext* ctx, uint64_t bytecodeOffset);

    // AWAIT on a non-promise value — throws RuntimeException; stashed in
    // pendingException by the existing helper convention.
    void jit_await_type_error(JitContext* ctx);
}
```

### 5. Critical: catch the deopt at the right place

`OSRDeoptException` is currently caught only at `OSRManager.cpp:364` —
inside the OSR loop entry path. AWAIT-deopt fires from a function-level
JIT-compiled body (not necessarily inside an OSR loop), so the throw must
unwind to a new catch site.

But there's a complication: the existing JIT runtime helpers
(`jit_call_function` at `JitHelpers_Arithmetic.cpp:182-186`, plus
`JitHelpers_Variables.cpp:94`, `JitHelpers_Arithmetic.cpp:241`, etc.) all
do `catch (...) { ctx->pendingException = std::current_exception(); }` —
they swallow exceptions and stash them. **An `OSRDeoptException` thrown
by JIT-emitted code inside a JIT function called from another JIT function
would be caught and stashed there, never reaching a top-level catch.**

Two-part fix:

**(a) Make `OSRDeoptException` propagate through helper catches.**
In every `catch (...)` site in JitHelpers_*.cpp, add a specific re-throw
arm *before* the `(...)` arm:

```cpp
catch (const OSRDeoptException&) { throw; }
catch (...) { ctx->pendingException = std::current_exception(); }
```

**(b) Add a function-level catch site for JIT-compiled async function entry.**
The existing entry path for OSR-compiled loops at `OSRManager.cpp:364`
catches the exception. For function-level JIT calls (e.g. when the
interpreter calls a JIT-compiled async function directly, or when one
JIT-compiled function calls another via `jit_call_function`), add an
analogous catch inside `jit_call_function` /`jit_call_function_fast`
(`JitHelpers_Arithmetic.cpp:156-187` and the following `_fast` variant)
so that an `OSRDeoptException` thrown from a JIT-compiled callee is
converted into "switch back to interpreter at the deopt IP, run the
callee's remaining instructions interpreted, return the result to the
caller." Concretely:

```cpp
catch (const OSRDeoptException& e)
{
    // Restore the JIT callee's locals into the interpreter callee frame,
    // set IP, and resume in the interpreter.
    DeoptState ds; ds.bytecodeOffset = e.bytecodeOffset; ds.locals = ...;
    DeoptimizationHandler::handleDeopt(ds, ..., *ctx->execContext);
    // Continue execution in interpreter — it picks up at the AWAIT.
    ctx->vm->resumeInterpreter();   // need to add this entry point
}
```

The `resumeInterpreter()` entry point is the second piece of new
infrastructure — a way to re-enter the interpreter loop at the current
`instructionPointer` from inside a JIT helper. The interpreter's main
loop already works from any starting IP, so this is essentially a
"public" wrapper around its existing dispatch loop, scoped to "run until
this frame returns."

This is the **non-trivial design point** of the plan. It replaces the
current implicit assumption (compiled functions never deopt mid-body) with
explicit handoff back to the interpreter at the deopt point.

### 6. Don't break OSR

Verify (read-only check) that the OSR loop catch at `OSRManager.cpp:364`
still handles AWAIT-deopt cleanly when an AWAIT inside an OSR-compiled loop
deopts. The same `OSRDeoptException` carries the IP; `handleDeopt`
restores state; the interpreter continues from that IP. No code change
expected — just a verification.

## Verification

1. **Existing tests pass under JIT**: full `mType --tests` run, especially
   the async suites (`testFiles/await/pass/`). Every existing async test
   exercises async function bodies that will now go through the JIT.
   Critical sub-tests:
   - `asyncAwaitInForLoop.mt`, `asyncAwaitInWhileLoop.mt` — sync-completion
     loops, should JIT cleanly with no deopts.
   - `asyncAwaitInLoopCondition.mt`, `asyncBreakInLoop.mt`,
     `asyncContinueInLoop.mt` — control flow + AWAIT.
   - `asyncExceptionAfterPartialExecution.mt`,
     `asyncExceptionInConstructor.mt`, etc. — exception paths must still
     work; pending-promise rejection should deopt-to-interpreter and run
     the existing reject/catch machinery.
   - `asyncDirectRecursion.mt`, `asyncCircularPromiseReference.mt` —
     recursive/cyclic promise patterns.

2. **JIT actually fires on async**: run with `--jit-stats` on
   `async_await_tight_loop.mt`. The compile count should include `compute`
   and `run`; the bailout count should be near-zero (every AWAIT hits the
   `PROMISE_INT` fast path).

3. **Deopt path correctness**: write a new test under
   `testFiles/await/pass/` where an async function awaits a promise that
   is *actually pending* (e.g. via a callback that resolves later). The
   JIT-compiled function must deopt into the interpreter and produce the
   same output as the interpreter-only baseline. Add this test before
   the JIT changes ship and confirm it passes.

4. **Wall-time win**: `mType --benchmark`. Targets:
   - `async_await_tight_loop.mt`: 1145 ms → expected 150-300 ms (4-7× faster).
   - `async_await_chain.mt`: 1774 ms → expected 250-500 ms.
   - 19 existing benchmarks: must not regress. Sync benchmarks don't go
     through the new opcodes, so any movement is run-to-run noise.

5. **`.mtc` round-trip**: serialize one async benchmark via
   `mType --compile -release async_await_tight_loop.mt` and run the
   `.mtc`. The new opcodes are already in MYT-202's serializable range;
   the JIT changes are runtime-only and don't affect bytecode format.

## What this plan deliberately does NOT do

- **Speculative inlining of awaitee bodies.** A natural follow-up
  (collapse `await compute(i)` to `compute`'s body inlined into `run`'s
  loop body, then fold the AWAIT to nothing). Builds on the MYT-210
  function-call inliner. Defer to a separate ticket.
- **Continuation-passing rewrite.** Out of scope; full state-machine
  transformation is months of work and not justified until sync-completion
  JIT lands.
- **JIT compilation of `executeAwait`'s suspend path.** Pending promises
  always deopt to the interpreter. The interpreter's existing event-loop /
  callback machinery handles the actual suspend/resume; the JIT just
  recognizes the fast path and bails on anything else.

## Effort estimate

~1.5-2 weeks of focused work. Roughly:

- Day 1: lift gate, extend supported opcodes, scaffold `JitCompiler_Async.cpp`.
- Days 2-3: emit cases for `CREATE_PROMISE` and the two fused variants
  (relatively mechanical — call helpers).
- Days 4-6: emit case for `AWAIT` with the inline-form fast path (the
  actual perf-critical asmjit work).
- Days 7-9: catch-site changes, `resumeInterpreter` entry point, getting
  deopt-from-JIT-callee correctness right. **This is where most of the
  surprise complexity will live.**
- Days 10+: test fixes, benchmark tuning, regression chase.
