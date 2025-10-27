// Error: @Script class without update method

@Script
class Player {
    int x;

    constructor() {
        x = 100;
    }

    // Missing: update(dt: float): void method
    function move(): void {
        x = x + 1;
    }
}

Player player = new Player();
