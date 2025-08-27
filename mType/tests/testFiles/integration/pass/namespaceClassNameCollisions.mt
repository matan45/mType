// Test name collision scenarios
namespace graphics {
    class Renderer {
        string type = "Graphics";
        
        function render(): string {
            return "Graphics rendering";
        }
    }
    
    final string TYPE = "Graphics";
}

namespace audio {
    class Renderer {
        string type = "Audio";
        
        function render(): string {
            return "Audio rendering";
        }
    }
    
    final string TYPE = "Audio";
}

namespace utils {
    // Same name as namespace above, but different context
    final string Graphics = "UtilsGraphics";
    final string Audio = "UtilsAudio";
    
    function processRenderer(string rendererType): string {
        if (rendererType == "graphics") {
            return Graphics;
        } else if (rendererType == "audio") {
            return Audio;
        }
        return "Unknown";
    }
}

// Test qualified access with name collisions
graphics::Renderer gfxRenderer = new graphics::Renderer();
audio::Renderer audioRenderer = new audio::Renderer();

print(gfxRenderer.render());
print(audioRenderer.render());
print(graphics::TYPE);
print(audio::TYPE);

string utilResult1 = utils::processRenderer("graphics");
string utilResult2 = utils::processRenderer("audio");
print(utilResult1);
print(utilResult2);

// Test using directives with potential conflicts
using namespace graphics;
Renderer defaultRenderer = new Renderer(); // Should use Graphics::Renderer
print(defaultRenderer.render());