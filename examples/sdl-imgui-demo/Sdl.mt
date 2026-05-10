// Phase 1 mType wrapper around the __native__sdl_* natives exposed by
// mtype-sdl-imgui-plugin. Window and Renderer are thin wrappers over the
// int handles minted by the plugin's HandleRegistry; calling .destroy()
// on either is safe to skip if you also call Sdl.quit() at shutdown.

class Window {
    public int handle;

    public constructor(int h) {
        this.handle = h;
    }

    public function destroy(): void {
        __native__sdl_destroy_window(this.handle);
    }
}

class Renderer {
    public int handle;

    public constructor(int h) {
        this.handle = h;
    }

    public function destroy(): void {
        __native__sdl_destroy_renderer(this.handle);
    }

    public function setDrawColor(int r, int g, int b, int a): void {
        __native__sdl_set_render_draw_color(this.handle, r, g, b, a);
    }

    public function clear(): void {
        __native__sdl_render_clear(this.handle);
    }

    public function present(): void {
        __native__sdl_render_present(this.handle);
    }
}

class Sdl {
    // Initialise SDL3 video + events. Throws SdlError on failure.
    public static function init(): bool {
        return __native__sdl_init();
    }

    public static function quit(): void {
        __native__sdl_quit();
    }

    public static function createWindow(string title, int width, int height): Window {
        return new Window(__native__sdl_create_window(title, width, height));
    }

    public static function createRenderer(Window window): Renderer {
        return new Renderer(__native__sdl_create_renderer(window.handle));
    }

    // Drains the next pending event into the plugin's event stash.
    // Returns the SDL event type code (0 if no event was pending).
    public static function pollEvent(): int {
        return __native__sdl_poll_event();
    }

    // Constant for SDL_EVENT_QUIT (resolved at runtime; differs SDL2 vs SDL3).
    public static function quitEventId(): int {
        return __native__sdl_event_quit_id();
    }

    // Raw pointer to the most-recently-polled SDL_Event, for handing to
    // ImGui.processEventPtr. Treat as opaque on the .mt side.
    public static function lastEventPtr(): int {
        return __native__sdl_event_ptr();
    }

    public static function lastError(): string {
        return __native__sdl_get_error();
    }

    public static function delay(int millis): void {
        __native__sdl_delay(millis);
    }
}
