// Test: @Script annotation without start method
// Expected: Error - @Script classes must have start(): void method

@Script
class Player {
    int x;

    constructor() {
        x = 0;
    }

    public function update(float dt): void {
        x = x + 1;
    }

    // Missing: start(): void

    public function clean(): void {}
}

Player p = new Player();
