// Test name collision scenarios
class GraphicsRenderer {
    string type = "Graphics";
    
    function render(): string {
        return "Graphics rendering";
    }
}

final string GRAPHICS_TYPE = "Graphics";

class AudioRenderer {
    string type = "Audio";
    
    function render(): string {
        return "Audio rendering";
    }
}

final string AUDIO_TYPE = "Audio";

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

// Test qualified access with name collisions
GraphicsRenderer gfxRenderer = new GraphicsRenderer();
AudioRenderer audioRenderer = new AudioRenderer();

print(gfxRenderer.render());
print(audioRenderer.render());
print(GRAPHICS_TYPE);
print(AUDIO_TYPE);

string utilResult1 = processRenderer("graphics");
string utilResult2 = processRenderer("audio");
print(utilResult1);
print(utilResult2);

// Test using directives with potential conflicts
// Note: Since namespaces removed, using GraphicsRenderer as default
GraphicsRenderer defaultRenderer = new GraphicsRenderer();
print(defaultRenderer.render());