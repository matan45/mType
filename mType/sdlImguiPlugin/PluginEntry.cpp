#include "PluginGlobals.hpp"

#include <SDL3/SDL.h>
#include <imgui.h>

namespace sdlimgui
{
    const MTypePluginHost* g_host = nullptr;
    HandleRegistry<SDL_Window>   g_windows;
    HandleRegistry<SDL_Renderer> g_renderers;
    HandleRegistry<ImGuiContext> g_imguiContexts;
    unsigned char g_lastEventBuffer[SDL_EVENT_BUFFER_BYTES] = {};
}

extern "C" MTYPE_PLUGIN_EXPORT
int mtype_plugin_register(uint32_t hostAbiVersion,
                          const MTypePluginHost* host,
                          MTypeContext* registrationCtx)
{
    if (hostAbiVersion != MTYPE_PLUGIN_ABI_VERSION) {
        return 1;
    }
    sdlimgui::g_host = host;

    sdlimgui::registerSdlNatives(registrationCtx);
    sdlimgui::registerImGuiNatives(registrationCtx);
    return 0;
}
