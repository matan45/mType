// Error: @Script class with wrong update method signature

@Script
class Player {
    int x;

    constructor() {
        x = 100;
    }

    // Wrong signature: should be update(dt: float): void
    // This has int parameter instead of float
    function update(int dt): void {
        x = x + dt;
    }
}

Player player = new Player();
