// Test: @Script annotation without clean method
// Expected: Error - @Script classes must have clean(): void method

@Script
class Player {
    int x;

    constructor() {
        x = 0;
    }

    public function update(float dt): void {
        x = x + 1;
    }

    public function start(): void {}

    // Missing: clean(): void
}

Player p = new Player();
