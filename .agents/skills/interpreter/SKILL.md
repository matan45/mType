---
name: interpreter
description: mType bytecode VM/runtime execution guidance for opcode semantics, VirtualMachine dispatch, stack/call-frame behavior, executor changes, runtime values/objects, async/interop, and JIT/no-JIT correctness. Use when fixing or changing interpreter/VM runtime behavior, execution regressions, bytecode instruction handling, stack or call-frame bugs, object/value runtime semantics, or JIT-vs-interpreter divergence.
---

# Interpreter

Use this skill for the bytecode VM execution layer. mType no longer has a tree-walking interpreter; "interpreter" means the VM/runtime path that executes `BytecodeProgram` instructions.

For runtime topology, opcode contracts, and verification gates, read [REFERENCE.md](REFERENCE.md).

## Boundaries

- Use [diagnose](../diagnose/SKILL.md) first when the symptom is not reproduced or minimized.
- Use [mtype-language-feature-tests](../mtype-language-feature-tests/SKILL.md) when the main task is authoring `.mt` fixtures or wiring suites.
- Use [runtime-perf](../runtime-perf/SKILL.md) when the main task is speed, JIT hot paths, inline caches, OSR, or perf regression measurement.
- Use [benchmark-coverage](../benchmark-coverage/SKILL.md) when no benchmark covers the runtime path being optimized.
- Use [refactor](../refactor/SKILL.md) when the task is behavior-preserving runtime restructuring.

## JIT Scope

This skill includes JIT correctness when JIT behavior must match the interpreter path: JIT-vs-`--no-jit` divergence, OSR/fallback execution, inline-cache opcode mutation, runtime-only opcode contracts, and VM state shared with JIT helpers. Route measurement-first optimization, tiering heuristics, codegen hot paths, and perf regressions to [runtime-perf](../runtime-perf/SKILL.md).

## Workflow

1. Build or confirm a feedback loop before editing. Prefer a targeted `.mt` fixture, existing suite, isolated executor test, or bytecode-program test.
2. Map the execution path before choosing files: source execution flows through `ScriptInterpreter -> BytecodeCompiler -> BytecodeExecutor -> VirtualMachine`; cached bytecode also goes through `BytecodeService` registration.
3. Classify the boundary: compiler emission, bytecode artifact contract, dispatch routing, executor behavior, VM loop state, runtime metadata, JIT/IC mutation, async/interop, or tests.
4. Inspect the smallest complete surface for that boundary. For opcode work, use the full opcode checklist in [REFERENCE.md](REFERENCE.md#opcode-change-checklist).
5. Keep `ExecutionContext` frame-local. Add VM-coupled state through explicit executor dependencies instead of hiding it in shared context.
6. Prefer the deepest stable test seam: isolated executor tests for local handlers, bytecode-program tests for opcode contracts, end-to-end `.mt` fixtures when compiler and runtime behavior must agree.
7. Verify both default JIT and `--no-jit` for runtime semantics unless the code path is proven JIT-independent.

## Land Gates

- Affected runtime suite passes in default JIT mode and `--no-jit`.
- `integration` passes in default JIT mode and `--no-jit` for call, object, array, async, interop, IC, OSR, or multi-program changes.
- Full `--tests` and `--tests --no-jit` are required for broad VM/compiler/runtime changes.
- Runtime-only opcodes, quickened opcodes, IC cached opcodes, and fused opcodes do not serialize into `.mtc` bytecode.
- No stack-object, lambda-capture, or GC-root lifetime change lands without checking the related lifetime paths in [REFERENCE.md](REFERENCE.md#lifetime-and-values).
