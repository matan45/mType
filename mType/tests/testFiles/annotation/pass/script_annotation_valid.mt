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

    // Required: update method
    public function update(float dt): void {
        x = x + 1;
    }

    // Required: start method
    public function start(): void {
        print("Player started");
    }

    // Required: clean method
    public function clean(): void {
        print("Player cleaned up");
    }

    public function getX(): int {
        return x;
    }
}

// Create and use the player
Player player = new Player();
player.update(0.016);
print(player.getX());
