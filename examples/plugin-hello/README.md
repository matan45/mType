# hello-from-plugin (MYT-289)

Minimal example of an mType native plugin: a C/C++ shared library that
registers a single native function (`__hello_greet`) callable from `.mt`
scripts via the runtime plugin loader.

## Build

The plugin is a standalone CMake project. It depends only on
`mType/plugin/PluginHostApi.h` (a single C header) — no link to the engine.

### Windows (MSVC)

```
cd examples\plugin-hello
cmake -B build -A x64
cmake --build build --config Release
```

Produces `build\Release\hello.dll`.

### Linux

```
cd examples/plugin-hello
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Produces `build/hello.so`.

### macOS

```
cd examples/plugin-hello
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Produces `build/hello.dylib`.

## Run

Copy or symlink the produced shared library next to `hello.mt`, then:

```
mType.exe hello.mt          # Windows
./mType hello.mt            # Linux/macOS
```

Expected output:

```
hello, world
hello, native plugin
12
<some integer — current count of registered functions>
```

The first two lines come from `__hello_greet`. The `12` line comes from
`__hello_apply_twice("double", 3)` — the plugin called the mType `double`
function twice (3 → 6 → 12) using `host->callFunction`. The last line is
the plugin enumerating every registered function (via
`host->listFunctions`) and counting them.

## How it works

1. `hello.mt` calls `__plugin_load("./hello.dll")` — built into the
   interpreter (`mType/plugin/PluginNatives.cpp`).
2. The loader opens the library (`LoadLibraryW` / `dlopen`) and resolves
   `mtype_plugin_register`.
3. The plugin's `mtype_plugin_register` validates the ABI version and calls
   `host->registerFunction("__hello_greet", &greet, nullptr)`.
4. From this point `__hello_greet` is a normal native function and can be
   called from any `.mt` code.
5. `__plugin_unload("./hello.dll")` removes the registered names from the
   runtime registry, invalidates inline-cache slots, and closes the library.

## Plugin author quick reference

The complete C ABI is in `mType/plugin/PluginHostApi.h`. Key rules:

- `MTypeValue*` pointers are valid only for the duration of the current
  native call. Copy primitive data out (via `getInt` / `getFloat` /
  `getString` / etc.) if you need to retain it.
- Errors: call `host->raiseError(ctx, type, message)` then return
  `host->makeNull(ctx)`. The host rethrows after your function returns —
  do NOT `throw` C++ exceptions across the DLL boundary.
- `mtype_plugin_register` must check `hostAbiVersion == MTYPE_PLUGIN_ABI_VERSION`
  and return non-zero on mismatch. The host will close the library on a
  non-zero return.
- Plugins run on the single VM thread.
- Plugins **may** call back into mType (ABI v2) via `host->callFunction` /
  `host->callMethod` — the inner call runs on the same thread; the VM
  saves/restores its IP and call stack across the inner execution.
