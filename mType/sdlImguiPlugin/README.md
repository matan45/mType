# mtype-sdl-imgui-plugin

A runtime-loaded native plugin (`mtype_sdl_imgui.dll` / `.so` / `.dylib`)
exposing SDL3 + ImGui (docking branch) to mType scripts.

This plugin uses only `mType/plugin/PluginHostApi.h` — it does **not** link
the engine. Loaded at runtime by `__plugin_load("./bin/.../mtype_sdl_imgui.dll")`.

## Phase status

This is **Phase 1**: enough surface to open a window, drive an event loop,
draw an `ImGui::Text` and a clickable `ImGui::Button`. Roadmap below.

| Phase | Surface | Status |
|---|---|---|
| 1 | init/quit · window · renderer · poll/quit event · ImGui context+backends · `Begin`/`End` · `Text` · `Button` | implemented |
| 2 | mouse / keyboard / text-input event extraction · slider/checkbox/combo/input · color picker | TODO |
| 3 | layout: columns · child windows · tabs · docking · splitters | TODO |
| 4 | textures (load PNG via SDL_image) · `Image` widget · custom fonts · clipboard | TODO |
| 5 | audio (SDL_mixer or SDL_audio raw) · gamepad / haptic | TODO |

Each phase adds ~20–30 native functions and the corresponding mType wrapper
methods. Phases are independent — Phase 2 can land before Phase 3 etc.

## Vendoring

The plugin's premake project is **gated** on `vendor/SDL3/` and
`vendor/imgui/` existing. If they're absent, premake silently skips the
project (so the rest of the workspace generates cleanly) and the runtime
test that loads `mtype_sdl_imgui.dll` will fail with a clear "file not
found" error. Do this once per checkout:

### 1. Vendor SDL3

Clone the SDL3 source (shallow clone keeps the checkout small):

```
git clone --depth=1 https://github.com/libsdl-org/SDL.git vendor/SDL3
```

Then build it once via its own CMake to produce the import lib + DLL:

```
cd vendor/SDL3
cmake -B build -A x64 -DSDL_TEST_LIBRARY=OFF
cmake --build build --config Release --parallel
```

Output (Windows):

```
vendor/SDL3/
├── include/SDL3/SDL.h     (headers, used at compile time)
└── build/Release/
    ├── SDL3.dll           (used at runtime — copy next to mType.exe)
    └── SDL3.lib           (import lib, linked by the plugin)
```

The premake project's `libdirs` already points at
`vendor/SDL3/build/Release` (and `…/Debug` for Debug builds), so once
this CMake build is done the workspace links cleanly.

Linux/macOS: same commands minus the `-A x64` flag. Output ends up at
`vendor/SDL3/build/libSDL3.so` (or `.dylib`).

### 2. Vendor ImGui (docking branch)

```
git clone --depth=1 --branch docking https://github.com/ocornut/imgui vendor/imgui
```

After cloning, the layout should be:

```
vendor/imgui/
├── imgui.h
├── imgui.cpp
├── imgui_demo.cpp
├── imgui_draw.cpp
├── imgui_tables.cpp
├── imgui_widgets.cpp
└── backends/
    ├── imgui_impl_sdl3.h
    ├── imgui_impl_sdl3.cpp
    ├── imgui_impl_sdlrenderer3.h
    └── imgui_impl_sdlrenderer3.cpp
```

Confirm the SDL3 backend files are present — older ImGui clones may
default to a non-docking branch that lacks them.

## Building

After vendoring:

```
runPremake.bat
```

Open `Interpreter.sln` and build the `mtype-sdl-imgui-plugin` project
(or build the whole solution). Output:

```
bin/mType/Release/x64/mtype_sdl_imgui.dll
bin/mType/Release/x64/SDL3.dll              ← copy this in manually
```

**Important on Windows**: `SDL3.dll` is loaded by the OS when
`mtype_sdl_imgui.dll` is loaded. Copy `vendor/SDL3/build/Release/SDL3.dll`
next to `mType.exe` (i.e. into `bin/mType/Release/x64/`) before running
the demo. premake doesn't do this automatically — keep it explicit so
reproducible builds don't accidentally rely on a stale system DLL.

```
copy vendor\SDL3\build\Release\SDL3.dll bin\mType\Release\x64\
```

## Running the demo

```
bin\mType\Release\x64\mType.exe examples\sdl-imgui-demo\demo.mt
```

You should see a 1280×720 window with one ImGui frame: a label, a click
counter, a "Click me" button, and a "Quit" button. Closing the window or
clicking Quit terminates cleanly.

## Adding bindings (Phase 2 and later)

Each new native function follows this shape:

```cpp
// In SdlBindings.cpp or ImGuiBindings.cpp:
MTypeValue* nMyNewBinding(void*, MTypeContext* ctx,
                           const MTypeValue* const* args, int argc) {
    if (!requireArgs(ctx, argc, /*expected=*/N, "__native__module_name")) return g_host->makeVoid(ctx);
    // Pull args via g_host->getInt / getFloat / getString / getBool.
    // Look up handles via g_windows.find / g_renderers.find / etc.
    // Call the SDL/ImGui function. Return via g_host->makeXxx.
}

// In registerSdlNatives / registerImGuiNatives:
reg("__native__module_name", &nMyNewBinding);
```

**Naming**: every plugin native must be registered under a `__native__`-
prefixed name. The mType compiler validates all called functions at
compile time; built-in natives are present then but plugin natives aren't.
The `__native__` prefix is the explicit opt-out that tells the compiler
the function resolves at runtime. Without it, every script that calls
your plugin's functions fails with "Function 'X' not found".

Then add a method to the corresponding mType wrapper class
(`Sdl.mt`/`ImGui.mt`) that calls `__module_name(...)` and returns a typed
result. Update the phase table at the top of this file when a phase is
complete.

## Cross-toolchain notes

The plugin uses `MTYPE_PLUGIN_ABI_VERSION` (currently 2) declared in
`PluginHostApi.h`. If the engine bumps it (e.g. adding more vtable
members), the plugin must be recompiled. The loader rejects mismatched
versions with a clear `PluginError` rather than crashing.
