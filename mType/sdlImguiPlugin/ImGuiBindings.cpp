/*
 * Phase 1 ImGui (docking branch) bindings.
 *
 * Phase 1 surface (~10):
 *   __imgui_create_context / __imgui_destroy_context
 *   __imgui_sdl3_init_for_renderer
 *   __imgui_sdl3_renderer_init
 *   __imgui_sdl3_process_event_ptr
 *   __imgui_sdl3_shutdown_renderer / __imgui_sdl3_shutdown
 *   __imgui_new_frame
 *   __imgui_render
 *   __imgui_begin / __imgui_end
 *   __imgui_text
 *   __imgui_button -> bool
 */

#include "PluginGlobals.hpp"

#include <SDL3/SDL.h>
#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>

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
                g_host->raiseError(ctx, "ImGuiError", m.c_str());
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

        MTypeValue* nImGuiCreateContext(void*, MTypeContext* ctx,
                                         const MTypeValue* const*, int argc)
        {
            if (!requireArgs(ctx, argc, 0, "__native__imgui_create_context")) return g_host->makeInt(ctx, 0);
            ImGuiContext* c = ImGui::CreateContext();
            if (!c) {
                g_host->raiseError(ctx, "ImGuiError", "ImGui::CreateContext returned null");
                return g_host->makeInt(ctx, 0);
            }
            ImGui::SetCurrentContext(c);
            return g_host->makeInt(ctx, g_imguiContexts.insert(c));
        }

        MTypeValue* nImGuiDestroyContext(void*, MTypeContext* ctx,
                                          const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_destroy_context")) return g_host->makeVoid(ctx);
            ImGuiContext* c = g_imguiContexts.erase(g_host->getInt(args[0]));
            if (c) ImGui::DestroyContext(c);
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nImGuiSdl3InitForRenderer(void*, MTypeContext* ctx,
                                               const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 2, "__native__imgui_sdl3_init_for_renderer")) return g_host->makeBool(ctx, 0);
            SDL_Window*   w = g_windows.find(g_host->getInt(args[0]));
            SDL_Renderer* r = g_renderers.find(g_host->getInt(args[1]));
            if (!w || !r) {
                g_host->raiseError(ctx, "ImGuiError",
                                   "__native__imgui_sdl3_init_for_renderer: invalid window or renderer id");
                return g_host->makeBool(ctx, 0);
            }
            return g_host->makeBool(ctx, ImGui_ImplSDL3_InitForSDLRenderer(w, r) ? 1 : 0);
        }

        MTypeValue* nImGuiSdl3RendererInit(void*, MTypeContext* ctx,
                                            const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_sdl3_renderer_init")) return g_host->makeBool(ctx, 0);
            SDL_Renderer* r = g_renderers.find(g_host->getInt(args[0]));
            if (!r) {
                g_host->raiseError(ctx, "ImGuiError",
                                   "__native__imgui_sdl3_renderer_init: invalid renderer id");
                return g_host->makeBool(ctx, 0);
            }
            return g_host->makeBool(ctx, ImGui_ImplSDLRenderer3_Init(r) ? 1 : 0);
        }

        MTypeValue* nImGuiSdl3ShutdownRenderer(void*, MTypeContext* ctx,
                                                const MTypeValue* const*, int)
        {
            ImGui_ImplSDLRenderer3_Shutdown();
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nImGuiSdl3Shutdown(void*, MTypeContext* ctx,
                                        const MTypeValue* const*, int)
        {
            ImGui_ImplSDL3_Shutdown();
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nImGuiSdl3ProcessEventPtr(void*, MTypeContext* ctx,
                                               const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_sdl3_process_event_ptr")) return g_host->makeVoid(ctx);
            int64_t addr = g_host->getInt(args[0]);
            const SDL_Event* ev = reinterpret_cast<const SDL_Event*>(addr);
            ImGui_ImplSDL3_ProcessEvent(ev);
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nImGuiNewFrame(void*, MTypeContext* ctx,
                                    const MTypeValue* const*, int)
        {
            ImGui_ImplSDLRenderer3_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nImGuiRender(void*, MTypeContext* ctx,
                                  const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_render")) return g_host->makeVoid(ctx);
            SDL_Renderer* r = g_renderers.find(g_host->getInt(args[0]));
            if (!r) return g_host->makeVoid(ctx);
            ImGui::Render();
            ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), r);
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nImGuiBegin(void*, MTypeContext* ctx,
                                 const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_begin")) return g_host->makeBool(ctx, 0);
            const char* name = getStr(args[0]);
            return g_host->makeBool(ctx, ImGui::Begin(name) ? 1 : 0);
        }

        MTypeValue* nImGuiEnd(void*, MTypeContext* ctx,
                               const MTypeValue* const*, int)
        {
            ImGui::End();
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nImGuiText(void*, MTypeContext* ctx,
                                const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_text")) return g_host->makeVoid(ctx);
            const char* s = getStr(args[0]);
            ImGui::TextUnformatted(s);
            return g_host->makeVoid(ctx);
        }

        MTypeValue* nImGuiButton(void*, MTypeContext* ctx,
                                  const MTypeValue* const* args, int argc)
        {
            if (!requireArgs(ctx, argc, 1, "__native__imgui_button")) return g_host->makeBool(ctx, 0);
            const char* label = getStr(args[0]);
            return g_host->makeBool(ctx, ImGui::Button(label) ? 1 : 0);
        }
    }

    void registerImGuiNatives(MTypeContext* ctx)
    {
        const auto reg = [&](const char* name, MTypeNativeFn fn) {
            g_host->registerFunction(ctx, name, fn, nullptr);
        };
        reg("__native__imgui_create_context",          &nImGuiCreateContext);
        reg("__native__imgui_destroy_context",         &nImGuiDestroyContext);
        reg("__native__imgui_sdl3_init_for_renderer",  &nImGuiSdl3InitForRenderer);
        reg("__native__imgui_sdl3_renderer_init",      &nImGuiSdl3RendererInit);
        reg("__native__imgui_sdl3_shutdown_renderer",  &nImGuiSdl3ShutdownRenderer);
        reg("__native__imgui_sdl3_shutdown",           &nImGuiSdl3Shutdown);
        reg("__native__imgui_sdl3_process_event_ptr",  &nImGuiSdl3ProcessEventPtr);
        reg("__native__imgui_new_frame",               &nImGuiNewFrame);
        reg("__native__imgui_render",                  &nImGuiRender);
        reg("__native__imgui_begin",                   &nImGuiBegin);
        reg("__native__imgui_end",                     &nImGuiEnd);
        reg("__native__imgui_text",                    &nImGuiText);
        reg("__native__imgui_button",                  &nImGuiButton);
    }
}
