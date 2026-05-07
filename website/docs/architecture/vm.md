---
title: Virtual Machine
sidebar_position: 3
---

# Virtual Machine

The VM (`mType/vm/runtime/`) is a stack-based execution engine.

## Key Components

- **Specialized instruction executors** — each instruction category has a dedicated executor (Single Responsibility Principle).
- **Call stack** — managed with overflow detection.
- **Async support** — cooperative scheduling on a built-in event loop.
- **Debugger hooks** — instrumented for breakpoints and stepping when run via `--debug`.

## Configuration

The VM exposes two tunable parameters:

| Parameter | Default | Notes |
|---|---|---|
| `DEFAULT_CALL_STACK_CAPACITY` | 64 frames | Initial reservation. |
| `DEFAULT_MAX_CALL_STACK_SIZE` | 1000 frames | Hard cap; aborts with a stack-trace error. |

```cpp
// Construct with a custom max depth
VirtualMachine vm(environment, /*maxStackDepth=*/2000);
```

## Stack Overflow

Hitting the depth limit aborts with a helpful error showing the call stack — usually a sign of unbounded recursion.

## Instruction Set

Every instruction is dispatched through an executor. Dispatch is **not** a `switch` over a function pointer table — executors aren't virtual, so the JIT can specialize directly for the common case (see [JIT](jit.md)).

## Class Metadata at Runtime

Bytecode files carry enough class metadata to support reflection and dynamic class lookup (`Class::forName`). The VM maintains a registry of loaded classes, methods, fields, and annotations; reflection APIs read directly from this registry.

## Async Scheduling

`async` functions return `Promise<T>`. The VM runs an event loop that polls promise queues and resumes suspended coroutines. `await` yields to the loop until the awaited promise resolves.

## See Also

- [Pipeline](pipeline.md)
- [JIT](jit.md)
- [Reflection](../stdlib/reflect.md)
- [Async / Await](../language/async-await.md)
