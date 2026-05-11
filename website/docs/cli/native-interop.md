---
title: Native Interop (FFN)
sidebar_position: 6
---

# Native Interop — Foreign Function Native (FFN)

mType exposes three bridges to/from C++:

| Direction | Mechanism | Used for |
|---|---|---|
| **mType → C++ (built-in)** | Native functions registered with `NativeRegistry` | Built-in I/O, math, networking, JSON; the standard library is built on this. |
| **mType → C++ (runtime plugin)** | `__plugin_load("./mything.dll")` loads a shared library that registers natives via a stable C ABI | Third-party bindings (graphics, ML, hardware) shipped without rebuilding the interpreter. |
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

## Runtime Native Plugins (`.dll` / `.so` / `.dylib`)

Built-in natives ship inside `mType.exe`. **Plugins** are native shared libraries loaded at runtime by an `.mt` script — third parties can package and distribute "mType-bindings-for-X" without rebuilding the interpreter.

The runtime exposes two natives:

| Function | Purpose |
|---|---|
| `__plugin_load(path: string): void` | Opens a `.dll`/`.so`/`.dylib`, calls its `mtype_plugin_register`, registers any natives it declares. Throws on architecture mismatch, missing entry point, or ABI version mismatch. |
| `__plugin_unload(path: string): void` | Removes the plugin's registered natives, invalidates inline-cache slots holding plugin function pointers, then closes the OS handle. |

### Plugin author quick start

A plugin is a single shared library that includes one C header (`mType/plugin/PluginHostApi.h`). It does **not** link against the engine — only the C ABI is shared, so a plugin built with one toolchain (MSVC vNNN, gcc, clang) loads cleanly into an engine built with another.

> **Naming convention**: plugin natives **must** be registered under names starting with `__native__`. The mType compiler validates every called function against the registry at compile time and rejects unknown names — but plugin natives are registered at runtime by `__plugin_load`, after compilation. The `__native__` prefix is the explicit opt-out: any name with this prefix is treated as runtime-resolved and skipped from compile-time existence checks. Built-in natives keep the regular `__name` convention (`__json_serialize`, `__plugin_load`, …) and remain compile-time validated. Pick `__native__yourmod_op` to match.


Minimal plugin (`hello.cpp`):

```cpp
#include "PluginHostApi.h"
#include <string>

static const MTypePluginHost* g_host = nullptr;

static MTypeValue* greet(void*, MTypeContext* ctx,
                         const MTypeValue* const* args, int argc) {
    if (argc != 1 || g_host->getTag(args[0]) != MT_TAG_STRING) {
        g_host->raiseError(ctx, "PluginError", "expected one string arg");
        return g_host->makeNull(ctx);
    }
    size_t n = 0;
    const char* name = g_host->getString(args[0], &n);
    std::string out = "hello, "; out.append(name, n);
    return g_host->makeString(ctx, out.c_str(), out.size());
}

extern "C" MTYPE_PLUGIN_EXPORT
int mtype_plugin_register(uint32_t hostAbiVersion,
                          const MTypePluginHost* host,
                          MTypeContext* registrationCtx) {
    if (hostAbiVersion != MTYPE_PLUGIN_ABI_VERSION) return 1;
    g_host = host;
    host->registerFunction(registrationCtx, "__native__hello_greet", &greet, nullptr);
    return 0;
}
```

Build it as a shared library that exports `mtype_plugin_register`. From `.mt` code:

```mtype
__plugin_load("./hello.dll")
print(__native__hello_greet("world"))   // -> "hello, world"
__plugin_unload("./hello.dll")
```

A complete buildable example with a CMake project lives at [`examples/plugin-hello/`](https://github.com/matan45/mType/tree/dev/examples/plugin-hello).

### The host C ABI

The `MTypePluginHost` struct is a function-pointer table (no C++ types cross the DLL boundary). The plugin uses it for every interaction with the engine:

| Group | Members |
|---|---|
| **Constructors** | `makeNull`, `makeVoid`, `makeBool`, `makeInt`, `makeFloat`, `makeString`, `makeArray(elemTag, length)`, `makeObject(className)` |
| **Inspection** | `getTag`, `getBool`, `getInt`, `getFloat`, `getString` |
| **Arrays (read+write)** | `arrayLen`, `arrayGet`, `arraySet` (mType arrays are fixed-size — choose `length` at `makeArray`) |
| **Objects (read+write)** | `objGet(obj, fieldName)`, `objSet(obj, fieldName, v)` — class must already be registered in the engine's `ClassRegistry` |
| **Registration** | `registerFunction(ctx, name, fn, userData)` — call from `mtype_plugin_register` only |
| **Errors** | `raiseError(ctx, type, message)` — non-throwing; the host trampoline rethrows after the plugin returns |
| **Existence checks** *(ABI v2)* | `hasClass(ctx, name)`, `hasFunction(ctx, name)` — `hasFunction` covers both global mType functions and registered natives |
| **Enumeration** *(ABI v2)* | `listClasses(ctx, cb, userData)`, `listFunctions(ctx, cb, userData)` — invoke `cb(userData, name)` once per registered name in alphabetical order |
| **Reentrancy** *(ABI v2)* | `callFunction(ctx, name, args, argc)`, `callMethod(ctx, receiver, name, args, argc)` — call back into mType from a plugin native; runs on the VM thread, saves/restores VM IP and call stack across the inner execution |

### Lifetime, errors, and ABI versioning

- **Value lifetime.** Every `MTypeValue*` handed to a plugin (input args, return values from `makeXxx` / `arrayGet` / `objGet`) is owned by a per-call host arena. Pointers are valid only for the duration of the current native call. To retain a primitive across calls, copy it out with `getInt`/`getFloat`/`getString`/etc.
- **Errors.** Do **not** `throw` C++ exceptions across the DLL boundary. Use `host->raiseError(ctx, type, message)`, then return immediately (`return host->makeNull(ctx);`). The host detects the pending error after your function returns and raises it into the calling `.mt` script as an mType exception.
- **ABI version.** `mtype_plugin_register`'s first argument is the host's ABI version. Plugins must check it equals `MTYPE_PLUGIN_ABI_VERSION` (defined in the header) and return non-zero on mismatch — the loader will close the library and report failure.

### Reentrancy

Plugin natives can call back into mType via `host->callFunction` / `host->callMethod` (ABI v2). The inner call runs on the same VM thread and saves/restores the VM's IP and call stack — the same machinery `ScriptAPI` uses for C++ → mType embedding. Errors thrown by the inner mType code are surfaced to the original `.mt` caller after the plugin's outer native returns; after a failed call the plugin should return promptly rather than continue.

```cpp
/* Look up an mType function by name, call it twice (f(f(value))) */
if (!host->hasFunction(ctx, funcName)) {
    host->raiseError(ctx, "PluginError", "no such function");
    return host->makeNull(ctx);
}
const MTypeValue* args1[] = { host->makeInt(ctx, value) };
MTypeValue* r1 = host->callFunction(ctx, funcName, args1, 1);
const MTypeValue* args2[] = { r1 };
return host->callFunction(ctx, funcName, args2, 1);
```

### Current limitations

- Plugins run on the single VM thread.
- No sandboxing: plugins run with full process privileges.
- No hot-reload, no manifest-based discovery — `__plugin_load` takes a literal absolute or CWD-relative path.

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
