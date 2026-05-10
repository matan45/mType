# sdl-imgui-demo

Minimal SDL3 + ImGui app written in mType, driven by the
`mtype-sdl-imgui-plugin` runtime plugin.

## Prerequisites

Build the plugin first. See
[`mType/sdlImguiPlugin/README.md`](../../mType/sdlImguiPlugin/README.md)
for vendoring (SDL3 + ImGui docking branch under `vendor/`) and build
steps.

After building, you should have:

```
bin/mType/Release/x64/mtype_sdl_imgui.dll
bin/mType/Release/x64/SDL3.dll              (Windows: copied manually)
```

## Run

From the project root:

```
bin\mType\Release\x64\mType.exe examples\sdl-imgui-demo\demo.mt
```

You should see a 1280×720 window with an ImGui panel containing a click
counter and two buttons. Click "Quit" or close the window to exit.

## Files

| File | Purpose |
|---|---|
| `Sdl.mt`   | mType wrapper around the `__sdl_*` natives (Window, Renderer, event poll). |
| `ImGui.mt` | mType wrapper around the `__imgui_*` natives (context, backends, Begin/End/Text/Button). |
| `demo.mt`  | The demo app — loads the plugin, opens a window, runs the event/render loop. |
