// Error: @Script class without default constructor

@Script
class Player {
    int x;

    // Only has constructor with parameters - missing default constructor
    constructor(int startX) {
        x = startX;
    }

    function update(float dt): void {
        x = x + 1;
    }
}

Player player = new Player(100);
