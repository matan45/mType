/*
 * Phase 1 SDL3 bindings (MYT-289 follow-up).
 *
 * Native names follow the mType "__module_op" convention. Handles (windows,
 * renderers) flow as int64 ids minted by HandleRegistry; native pointers
 * never cross the C ABI boundary.
 *
 * Phase 1 surface (15-ish):
 *   __sdl_init / __sdl_quit
 *   __sdl_create_window / __sdl_destroy_window
 *   __sdl_create_renderer / __sdl_destroy_renderer
 *   __sdl_set_render_draw_color
 *   __sdl_render_clear / __sdl_render_present
 *   __sdl_poll_event              -> int (event type, 0 if no pending event)
 *   __sdl_event_quit_id           -> int (constant for SDL_EVENT_QUIT)
 *   __sdl_get_error               -> string
 *   __sdl_delay                   -> void (millis)
 *   __sdl_event_ptr               -> int (raw &g_lastEventBuffer for ImGui_ImplSDL3_ProcessEvent)
 */

#include "PluginGlobals.hpp"

#include <SDL3/SDL.h>
#include <cstdint>
#include <cstring>
#include <string>

namespace sdlimgui
{
    namespace
    {
        bool requireArgs(MTypeContext* ctx, int argc, int expected, const char* name)
        {
            if (argc != expected) {
                std::string m = std::string(name) + ": expected " + std::to_string(expected)
                              + " args, got " + std::to_string(argc);
                g_host->raiseError(ctx, "SdlError", m.c_str());
                return false;
            }
            return true;
        }

        const char* getStr(const MTypeValue* v, size_t* outLen = nullptr)
        {
            if (g_host->getTag(v) != MT_TAG_STRING) {
                if (outLen) *outLen = 0;
                return "";
            }
            return g_host->getString(v, outLen);
        }

        /* ---------------------------------------------------------------- */

        MTypeValue* nSdlInit(void*, MTypeContext* ctx, const MTypeValue* const*, int argc)
        {
            if (!requireArgs(ctx, argc, 0, "__native__sdl_init")) return g_host->makeNull(ctx);
            int rc = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) ? 0 : 1;
            if (rc != 0) {
                std::string m = std::string("SDL_Init failed: ") + SDL_GetError();
                g_host->raiseError(ctx, "SdlError", m.c_str());
            }
            return g_host->makeBool(ctx, rc == 0);
        }

        MTypeValue* nSdlQuit(void*, MTypeContext* ctx, const MTypeValue* const*, int)
        {
            SDL_Quit();
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nSdlCreateWindow(void*, MTypeContext* ctx,
                                      const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 3, "__native__sdl_create_window")) return g_host->makeInt(ctx, 0);
            const char* title = getStr(args[0]);
            int w = static_cast<int>(g_host->getInt(args[1]));
            int h = static_cast<int>(g_host->getInt(args[2]));
            SDL_Window* win = SDL_CreateWindow(title, w, h, SDL_WINDOW_RESIZABLE);
            if (!win) {
                std::string m = std::string("SDL_CreateWindow failed: ") + SDL_GetError();
                g_host->raiseError(ctx, "SdlError", m.c_str());
                return g_host->makeInt(ctx, 0);
            }
            return g_host->makeInt(ctx, g_windows.insert(win));
        }

        MTypeValue* nSdlDestroyWindow(void*, MTypeContext* ctx,
                                       const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sdl_destroy_window")) return g_host->makeVoid(ctx);
            SDL_Window* win = g_windows.erase(g_host->getInt(args[0]));
            if (win) SDL_DestroyWindow(win);
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nSdlCreateRenderer(void*, MTypeContext* ctx,
                                        const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sdl_create_renderer")) return g_host->makeInt(ctx, 0);
            SDL_Window* win = g_windows.find(g_host->getInt(args[0]));
            if (!win) {
                g_host->raiseError(ctx, "SdlError", "__native__sdl_create_renderer: invalid window id");
                return g_host->makeInt(ctx, 0);
            }
            SDL_Renderer* r = SDL_CreateRenderer(win, nullptr);
            if (!r) {
                std::string m = std::string("SDL_CreateRenderer failed: ") + SDL_GetError();
                g_host->raiseError(ctx, "SdlError", m.c_str());
                return g_host->makeInt(ctx, 0);
            }
            return g_host->makeInt(ctx, g_renderers.insert(r));
        }

        MTypeValue* nSdlDestroyRenderer(void*, MTypeContext* ctx,
                                         const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sdl_destroy_renderer")) return g_host->makeVoid(ctx);
            SDL_Renderer* r = g_renderers.erase(g_host->getInt(args[0]));
            if (r) SDL_DestroyRenderer(r);
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nSdlSetRenderDrawColor(void*, MTypeContext* ctx,
                                            const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 5, "__native__sdl_set_render_draw_color")) return g_host->makeVoid(ctx);
            SDL_Renderer* r = g_renderers.find(g_host->getInt(args[0]));
            if (!r) return g_host->makeVoid(ctx);
            SDL_SetRenderDrawColor(r,
                                   static_cast<Uint8>(g_host->getInt(args[1])),
                                   static_cast<Uint8>(g_host->getInt(args[2])),
                                   static_cast<Uint8>(g_host->getInt(args[3])),
                                   static_cast<Uint8>(g_host->getInt(args[4])));
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nSdlRenderClear(void*, MTypeContext* ctx,
                                     const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sdl_render_clear")) return g_host->makeVoid(ctx);
            SDL_Renderer* r = g_renderers.find(g_host->getInt(args[0]));
            if (r) SDL_RenderClear(r);
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nSdlRenderPresent(void*, MTypeContext* ctx,
                                       const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sdl_render_present")) return g_host->makeVoid(ctx);
            SDL_Renderer* r = g_renderers.find(g_host->getInt(args[0]));
            if (r) SDL_RenderPresent(r);
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nSdlPollEvent(void*, MTypeContext* ctx,
                                   const MTypeValue* const*, int argc)
        {
            if (!requireArgs(ctx, argc, 0, "__native__sdl_poll_event")) return g_host->makeInt(ctx, 0);
            static_assert(sizeof(SDL_Event) <= SDL_EVENT_BUFFER_BYTES,
                          "SDL_Event grew beyond the plugin's stash buffer; bump SDL_EVENT_BUFFER_BYTES");
            SDL_Event* ev = reinterpret_cast<SDL_Event*>(g_lastEventBuffer);
            std::memset(g_lastEventBuffer, 0, SDL_EVENT_BUFFER_BYTES);
            if (!SDL_PollEvent(ev)) {
                return g_host->makeInt(ctx, 0);  /* 0 == no event */
            }
            return g_host->makeInt(ctx, static_cast<int64_t>(ev->type));
        }

        MTypeValue* nSdlEventQuitId(void*, MTypeContext* ctx,
                                     const MTypeValue* const*, int)
        {
            return g_host->makeInt(ctx, static_cast<int64_t>(SDL_EVENT_QUIT));
        }

        MTypeValue* nSdlEventPtr(void*, MTypeContext* ctx,
                                  const MTypeValue* const*, int)
        {
            /* Raw address of the stash event for ImGui_ImplSDL3_ProcessEvent. */
            return g_host->makeInt(ctx, reinterpret_cast<int64_t>(g_lastEventBuffer));
        }

        MTypeValue* nSdlGetError(void*, MTypeContext* ctx,
                                  const MTypeValue* const*, int)
        {
            const char* err = SDL_GetError();
            return g_host->makeString(ctx, err ? err : "", err ? std::strlen(err) : 0);
        }

        MTypeValue* nSdlDelay(void*, MTypeContext* ctx,
                               const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__sdl_delay")) return g_host->makeVoid(ctx);
            SDL_Delay(static_cast<Uint32>(g_host->getInt(args[0])));
            return g_host->makeVoid(ctx);
        }
    }

    void registerSdlNatives(MTypeContext* ctx)
    {
        const auto reg = [&](const char* name, MTypeNativeFn fn) {
            g_host->registerFunction(ctx, name, fn, nullptr);
        };
        reg("__native__sdl_init",                  &nSdlInit);
        reg("__native__sdl_quit",                  &nSdlQuit);
        reg("__native__sdl_create_window",         &nSdlCreateWindow);
        reg("__native__sdl_destroy_window",        &nSdlDestroyWindow);
        reg("__native__sdl_create_renderer",       &nSdlCreateRenderer);
        reg("__native__sdl_destroy_renderer",      &nSdlDestroyRenderer);
        reg("__native__sdl_set_render_draw_color", &nSdlSetRenderDrawColor);
        reg("__native__sdl_render_clear",          &nSdlRenderClear);
        reg("__native__sdl_render_present",        &nSdlRenderPresent);
        reg("__native__sdl_poll_event",            &nSdlPollEvent);
        reg("__native__sdl_event_quit_id",         &nSdlEventQuitId);
        reg("__native__sdl_event_ptr",             &nSdlEventPtr);
        reg("__native__sdl_get_error",             &nSdlGetError);
        reg("__native__sdl_delay",                 &nSdlDelay);
    }
}
