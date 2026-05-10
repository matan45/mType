// Phase 1 mType wrapper around the __native__imgui_* natives exposed by
// mtype-sdl-imgui-plugin. Tracks one active ImGui context as an int
// handle. The SDL3 backends are stateful singletons inside ImGui itself,
// so initSdl3ForRenderer / initSdl3Renderer don't return handles.

import * from "Sdl.mt";

class ImGuiContext {
    public int handle;

    public constructor(int h) {
        this.handle = h;
    }

    public function destroy(): void {
        __native__imgui_destroy_context(this.handle);
    }
}

class ImGui {
    public static function createContext(): ImGuiContext {
        return new ImGuiContext(__native__imgui_create_context());
    }

    // Bind ImGui's SDL3 platform + renderer backends to the supplied
    // window/renderer. Call createContext() first.
    public static function initSdl3ForRenderer(Window window, Renderer renderer): bool {
        return __native__imgui_sdl3_init_for_renderer(window.handle, renderer.handle);
    }

    public static function initSdl3Renderer(Renderer renderer): bool {
        return __native__imgui_sdl3_renderer_init(renderer.handle);
    }

    public static function shutdownSdl3(): void {
        __native__imgui_sdl3_shutdown_renderer();
        __native__imgui_sdl3_shutdown();
    }

    // Forward the most-recent SDL event into ImGui's input pipeline.
    // Call from the event loop after Sdl.pollEvent() returned non-zero.
    public static function processSdlEvent(): void {
        __native__imgui_sdl3_process_event_ptr(Sdl::lastEventPtr());
    }

    public static function newFrame(): void {
        __native__imgui_new_frame();
    }

    public static function render(Renderer renderer): void {
        __native__imgui_render(renderer.handle);
    }

    // Begin a top-level window. Always pair with end().
    public static function begin(string title): bool {
        return __native__imgui_begin(title);
    }

    public static function end(): void {
        __native__imgui_end();
    }

    public static function text(string label): void {
        __native__imgui_text(label);
    }

    // Draw a labeled button. Returns true on the frame the user clicks it.
    public static function button(string label): bool {
        return __native__imgui_button(label);
    }
}
