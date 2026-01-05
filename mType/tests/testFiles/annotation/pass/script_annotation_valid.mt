// Valid @Script class with all requirements

@Script
class Player {
    int x;
    int y;

    // Required: default constructor
    constructor() {
        x = 100;
        y = 100;
    }

    // Required: onUpdate method
    public function onUpdate(float dt): void {
        x = x + 1;
    }

    // Required: onStart method
    public function onStart(): void {
        print("Player started");
    }

    // Required: onDestroy method
    public function onDestroy(): void {
        print("Player destroyed");
    }

    public function getX(): int {
        return x;
    }
}

// Create and use the player
Player player = new Player();
player.onUpdate(0.016);
print(player.getX());
