// Test final variables across different scopes
namespace constants {
    final int MAX_LEVEL = 100;
    final string GAME_NAME = "TestGame";
    
    class Settings {
        final int DEFAULT_VOLUME = 75;
        static final string VERSION = "1.0.0";

        constructor() {
            // Constructor logic if needed
        }
        
        function getMaxLevel(): int {
            return MAX_LEVEL; // Access namespace final
        }
        
        function getGameInfo(): string {
            return GAME_NAME + " v" + VERSION;
        }
        
        static function getDefaultVolume(): int {
            return 75; // Can't access instance final from static
        }
    }
    
    namespace audio {
        final float DEFAULT_PITCH = 1.0;
        
        class AudioManager {
            final int CHANNELS = 32;
            
            function configure(): string {
                return "Audio: " + toString(CHANNELS) + " channels, pitch: " + toString(DEFAULT_PITCH);
            }
        }
    }
}

// Test final variable access across scopes
constants::Settings settings = new constants::Settings();
constants::audio::AudioManager audio2 = new constants::audio::AudioManager();

print(settings.getMaxLevel());
print(settings.getGameInfo());
print(constants::Settings::getDefaultVolume());
print(audio2.configure());