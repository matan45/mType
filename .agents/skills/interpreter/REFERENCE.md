# Interpreter Reference

Use this reference after loading [SKILL.md](SKILL.md). Keep changes narrow, but inspect every side of the runtime contract touched by the behavior.

## Runtime Topology

- Source execution: `mType/services/ScriptInterpreter.cpp` parses, optimizes, compiles, and executes.
- Cached bytecode execution: `BytecodeService` loads `.mtc`, registers metadata, then hands the program to execution.
- Bytecode artifact: `mType/vm/bytecode/BytecodeProgram.*`, `Instruction`, constants, class metadata, source locations, side tables, serializer/deserializer.
- Opcode catalog: `mType/vm/bytecode/OpCode.hpp`.
- VM core: `mType/vm/runtime/VirtualMachine.hpp`, `VirtualMachineLoop.cpp`, `VirtualMachineDispatch*.cpp`, `VirtualMachineAsync.cpp`, `VirtualMachineJit*.cpp`.
- Execution state: `mType/vm/runtime/context/ExecutionContext.hpp`, `mType/vm/runtime/stack/StackManager.hpp`, `CallFrame`, `SharedStackFrame`.
- Opcode handlers: `mType/vm/runtime/executors/`, especially `FunctionExecutor`, `ObjectExecutor`, `VariableExecutor`, `ArrayExecutor`, `TypeExecutor`, and `InlineCacheExecutor`.
- Runtime metadata: `mType/environment/Environment.hpp`, `environment/registry/TypeCatalog.hpp`, `ClassRegistry`, `FunctionRegistry`, `NativeRegistry`.
- Runtime optimization/JIT: `mType/vm/optimization/`, `mType/services/BytecodeOptimizationService.*`, `mType/vm/jit/`, `mType/vm/jit/ic/`.

## Opcode Change Checklist

For any new, removed, or behavior-changing opcode, inspect and update the full contract:

- `OpCode.hpp` enum, comments, and serializable vs runtime-only intent.
- `BytecodeProgram` operand validation, instruction naming/disassembly, serializer, and deserializer.
- Compiler emission in `mType/vm/compiler/` and any bytecode optimization pass that rewrites the instruction stream.
- Dispatch routing in `VirtualMachineDispatch*.cpp`.
- Relevant executor handler and fallback behavior when JIT, IC, or quickening is disabled.
- JIT compiler/helper support when the opcode can appear in hot paths or OSR loops.
- Tests at the lowest stable seam plus an end-to-end fixture when compiler and runtime behavior must agree.

## Runtime-Only Opcode Contract

Treat runtime opcode mutation as a serialization boundary.

- Cached method/field opcodes, local quickening, IC fused opcodes, and deopt-specialized instructions are runtime-only unless the bytecode artifact explicitly says otherwise.
- Runtime-only opcodes must not be emitted by `BytecodeCompiler` as serialized source output.
- `.mtc` deserialization must reject runtime-only opcodes that would require live side-table state.
- Side-table state such as `CachedInstructionState` must stay synchronized with opcode promotion, deopt, fallback, and instruction pointer behavior.
- If IC is disabled, fused/cached opcode fallbacks must replay equivalent generic behavior or fail with the same user-visible error.

## Cross-Path Semantics

Call, return, exception, await, frame, and stack behavior can run through more than one loop.

- Normal script execution uses `VirtualMachine::interpretLoop`.
- Direct invocation uses `executeFunction`, `invokeMethod`, `invokeStaticMethod`, and lambda invocation paths.
- Async interop uses `driveAsyncInvocation` and continuation-driven resumes.
- JIT fallback and helper paths can enter mini-interpretation and must not recursively re-enter JIT dispatch unsafely.
- Exception and finally behavior must preserve pending exception suppression, source locations, debugger hooks, and frame unwind cleanup.
- Multi-program calls must use the active `executionCtx->program`, `loadedPrograms`, and `programIndex` instead of assuming the main `program` pointer is current.

## Lifetime And Values

Review lifetime paths together; these bugs rarely live in one file.

- Stack-promoted objects use `NEW_STACK`, `STACK_OBJECT`, `CallFrame::stackObjects`, and `CallFrame::releaseStackObjects()`.
- Per-scope stack-object release uses `STACK_SCOPE_ENTER` and `STACK_SCOPE_LEAVE`.
- Lambda capture uses `SharedStackFrame`; captured values must not borrow stack-promoted objects after the producing frame exits.
- Object allocation and field/method access involve `ObjectExecutor*`, `ObjectInstancePool`, `TypeCatalog`, class metadata, and inline cache state.
- Array views, value classes, boxed primitives, string interning, and aliasing need both value semantics and GC-root behavior checked.
- GC root collection must include every live stack, frame, object, lambda, async promise, and interop reference touched by the change.

## Verification Matrix

- Narrow executor behavior: run the owning suite and any isolated executor or callback test in both default JIT and `--no-jit` when applicable.
- Opcode or bytecode contract change: add or update a bytecode-program test, then run `--test bytecode-optimization` if relevant.
- Compiler plus runtime semantic change: add or update `.mt + .expected` fixtures and run the affected suite in both modes.
- IC, OSR, JIT fallback, function call, field/method dispatch, object, array, async, interop, or multi-program change: run `--test integration` and `--test integration --no-jit`.
- Broad VM/compiler/runtime change: run `--tests` and `--tests --no-jit`.
- Performance-sensitive script addition: register the benchmark in both `mType/run/BenchmarkRunner_Internal.hpp` and `mType/tests/suites/IntegrationTestSuite.cpp`.

Common Windows commands from repo root:

```powershell
bin\mType\Release\x64\mType.exe --test integration
bin\mType\Release\x64\mType.exe --test integration --no-jit
bin\mType\Release\x64\mType.exe --tests
bin\mType\Release\x64\mType.exe --tests --no-jit
```
