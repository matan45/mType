// Phase 1 SDL3 + ImGui demo.
//
// Run from the project root:
//   mType.exe examples/sdl-imgui-demo/demo.mt
//
// (See README.md for vendoring + build steps before this can run.)

import * from "Sdl.mt";
import * from "ImGui.mt";

__plugin_load("./bin/mType/Release/x64/mtype_sdl_imgui.dll");

Sdl::init();

Window window = Sdl::createWindow("mType + SDL3 + ImGui demo", 1280, 720);
Renderer renderer = Sdl::createRenderer(window);

ImGuiContext gui = ImGui::createContext();
ImGui::initSdl3ForRenderer(window, renderer);
ImGui::initSdl3Renderer(renderer);

int clickCount = 0;
bool running = true;

while (running) {
    int eventType = Sdl::pollEvent();
    while (eventType != 0) {
        ImGui::processSdlEvent();
        if (eventType == Sdl::quitEventId()) {
            running = false;
        }
        eventType = Sdl::pollEvent();
    }

    ImGui::newFrame();

    if (ImGui::begin("Hello from mType")) {
        ImGui::text("This UI is rendered by ImGui via the runtime plugin loader.");
        ImGui::text("Clicks so far: " + clickCount);
        if (ImGui::button("Click me")) {
            clickCount = clickCount + 1;
        }
        if (ImGui::button("Quit")) {
            running = false;
        }
    }
    ImGui::end();

    renderer.setDrawColor(30, 30, 30, 255);
    renderer.clear();
    ImGui::render(renderer);
    renderer.present();
}

ImGui::shutdownSdl3();
gui.destroy();
renderer.destroy();
window.destroy();
Sdl::quit();
__plugin_unload("./bin/mType/Release/x64/mtype_sdl_imgui.dll");
