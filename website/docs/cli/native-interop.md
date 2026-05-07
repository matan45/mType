---
title: Native Interop (FFN)
sidebar_position: 6
---

# Native Interop — Foreign Function Native (FFN)

mType exposes a two-way bridge with C++:

| Direction | Mechanism | Used for |
|---|---|---|
| **mType → C++** | Native functions registered with `NativeRegistry` | Built-in I/O, math, networking, JSON; the standard library is built on this. |
| **C++ → mType** | `@Script` annotation + `ScriptAPI` from the embedding host | Game engines, plugins, or any C++ host that wants to drive mType objects. |

## Calling C++ from mType

The standard library is wired this way: `print`, `sqrt`, `sin/cos/tan`, `strLength`, `parsePrimitive`, `hashCode`, the JSON module, and the networking module are all C++ functions exposed as ordinary mType callables.

### C++ side — register a native

`NativeRegistry::registerNativeFunction(name, fn)` adds a function to the runtime. The `NativeBinder::bind` helper auto-converts ordinary C++ signatures (`double sqrt_fn(double x)`, `int64_t strLength_fn(const std::string&)`, etc.) into the engine's `Value` calling convention.

```cpp
// mType/environment/registry/builtin/BuiltinNatives.cpp
static double sqrt_fn(double x) { return std::sqrt(x); }
static int64_t strLength_fn(const std::string& s) {
    return static_cast<int64_t>(s.length());
}

void registerBuiltins(NativeRegistry& registry) {
    registry.registerNativeFunction("sqrt",      NativeBinder::bind("sqrt",      &sqrt_fn));
    registry.registerNativeFunction("strLength", NativeBinder::bind("strLength", &strLength_fn));
}
```

For functions that need direct access to the runtime (e.g. to allocate objects or schedule async work), pass a `NativeFunction` taking `(NativeContext, args)` directly:

```cpp
registry->registerNativeFunction(
    "__json_serialize",
    { /*userData=*/nullptr, /*thunk=*/__json_serialize });
```

### mType side — call the native

Native functions are called like any other mType function. The standard library wraps low-level natives in higher-level classes (e.g. `Json`, `Http`, `Random`) so user code never sees the underscore-prefixed names directly. If you're adding a new native binding, the stdlib convention is to give it a `__name` C++ identifier and wrap it from a regular `.mt` class:

```mtype
// lib/math/Random.mt (sketch)
public class Random {
    public function nextInt(int min, int max): int {
        return __random_nextInt(min, max);   // resolves to the C++ binding
    }
}
```

The standard library's source is in [matan45/mTypeLib](https://github.com/matan45/mTypeLib); the C++ side lives under `mType/environment/registry/builtin/`, `mType/json/`, `mType/net/`, and `mType/reflection/`.

## Calling mType from C++

Mark the class with `@Script`, then drive it from C++ via the `ScriptAPI` facade.

### Mark the mType class

```mtype
@Script
public class GameLogic {
    public constructor() { }

    public function onStart(): void { print("started"); }
    public function onUpdate(float deltaTime): void { /* ... */ }
    public function onDestroy(): void { }

    // Async methods are scheduled on the VM's event loop when called from C++.
    public function async onInteropTest(string msg): Promise<void> {
        print("before await: " + msg);
        String result = await someAsyncOp(msg);
        print("after await: " + result);
    }
}
```

Any class can be `@Script`-tagged — the only requirement is that it's `public`.

### Drive it from C++

The host constructs a `ScriptAPI` over an `Environment` and a `VirtualMachine`, then locates classes, instantiates them, and invokes methods:

```cpp
services::ScriptAPI api(env, vm, program);

// Find an @Script class and instantiate it
value::Value gameObj = api.newInstance("GameLogic", /*ctorArgs=*/{});

// Call methods (sync return)
api.callMethod(gameObj, "onStart", {});

// Async methods: scheduled on the event loop, return a Promise<T> Value
value::Value promise = api.callMethod(gameObj, "onInteropTest", {
    value::Value(std::string("from C++"))
});
// Pump the event loop until the promise settles.
```

`callFunction(name, args)` runs free functions; `callMethod(obj, name, args)` dispatches on a specific instance. Async methods are detected automatically and scheduled via the VM's event loop — the call returns a promise `Value` representing the in-flight task.

## CLI Helpers

Two flags help you inspect and exercise the embedding surface without writing C++ code:

| Flag | Purpose |
|---|---|
| `mType --find-script-classes <file.mt>` | Lists every `@Script` class declared in the file (and its imports). |
| `mType --test-script-objects <file.mt>` | Demo: creates each `@Script` object and calls a few standard methods (`onStart`, `onUpdate`, etc.) from C++. |

Use the second one as a smoke test when adding `@Script` classes.

## See Also

- [Annotations — `@Script`](../language/annotations.md)
- [Async / Await](../language/async-await.md) — async methods called from C++ are scheduled on the VM event loop
- [Reflection](../stdlib/reflect.md) — for discovering classes and methods at runtime from inside mType
- [`languageserver/README.md`](https://github.com/matan45/mType/blob/dev/languageserver/README.md) — the LSP server's embedding pattern is a working example of `ScriptAPI` use
