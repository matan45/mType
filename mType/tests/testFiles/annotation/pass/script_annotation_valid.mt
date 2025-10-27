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

    public function getX(): int {
        return x;
    }
}

// Create and use the player
Player player = new Player();
player.update(0.016);
print(player.getX());
