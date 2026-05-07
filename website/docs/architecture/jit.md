---
title: JIT Compiler
sidebar_position: 4
---

# JIT Compiler

The optional JIT (`mType/vm/jit/`) uses [AsmJit](https://asmjit.com) to emit x86-64 native code for hot bytecode methods. JIT is **on by default**; pass `--no-jit` to disable it.

## Status

The JIT scaffolding is in place: tiering, inline caches, on-stack replacement (OSR), and deoptimization. Multiple completed phases have landed as specialized fast paths (e.g. `INVOKE_INT`, `INVOKE_FLOAT`, OSR removal of the deny-list). Some phases remain in progress — see commit history under `mType/vm/jit/`.

## Why Use a JIT

- The interpreter dispatches every instruction; the JIT compiles a sequence of instructions into native code, eliminating dispatch overhead.
- Inline caches specialize call sites for the receiver type, turning megamorphic dispatch into direct branches.
- OSR lets a long-running interpreted loop transfer to JIT-compiled code mid-execution.

## When the JIT Kicks In

A method becomes a JIT candidate after a configurable hotness threshold. The compiled code lives alongside the bytecode; the VM dispatches to it via a small thunk on subsequent calls.

## Inline Caches and Quickening

Common shape-stable call patterns (e.g. method invoked on the same class repeatedly) get specialized:

- **Monomorphic** — single observed type, direct call.
- **Polymorphic** — small set of types, table dispatch.
- **Megamorphic** — fall back to the generic dispatch path.

The bytecode supports cached opcodes (`CALL_METHOD_CACHED`) that the JIT can lower to a direct call when the receiver type is known and stable.

## Deoptimization

When an assumption breaks — for example, a previously-stable receiver type changes — the JIT deopts back to the interpreter at the safest instruction boundary. The bytecode stream and call stack are designed so this transfer is correct without state-loss.

## Inspection

```bash
mType --jit-stats <script.mt>
```

prints JIT compilation counts and the time spent in JITed code after the script finishes.

## Limitations

The JIT favors code that touches primitives and small monomorphic call sites. Highly polymorphic or boxing-heavy paths can stay in the interpreter. See [Reference / Limitations](../reference/limitations.md) for known JIT corner cases.

## See Also

- [Virtual Machine](vm.md)
- [CLI / Diagnostics](../cli/reference.md#diagnostics)
