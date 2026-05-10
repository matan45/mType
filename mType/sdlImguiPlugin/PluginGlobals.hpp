#pragma once
/*
 * Plugin-global state shared across the binding TUs:
 *  - the host vtable pointer (set in mtype_plugin_register)
 *  - one HandleRegistry per native type
 *
 * Definitions live in PluginEntry.cpp.
 */

#include "PluginHostApi.h"
#include "HandleRegistry.hpp"

struct SDL_Window;
struct SDL_Renderer;
struct ImGuiContext;

namespace sdlimgui
{
    extern const MTypePluginHost* g_host;

    extern HandleRegistry<SDL_Window>   g_windows;
    extern HandleRegistry<SDL_Renderer> g_renderers;
    extern HandleRegistry<ImGuiContext> g_imguiContexts;

    /* Most recently polled SDL_Event. Stored in a fixed-size byte buffer so
     * this header doesn't need to include <SDL3/SDL.h>. The actual SDL_Event
     * size is well under 64 bytes; the buffer is intentionally generous. */
    constexpr int SDL_EVENT_BUFFER_BYTES = 128;
    extern unsigned char g_lastEventBuffer[SDL_EVENT_BUFFER_BYTES];

    void registerSdlNatives(MTypeContext* ctx);
    void registerImGuiNatives(MTypeContext* ctx);
}
