// Test: @Script annotation on classes
// Expected: Pass - classes are marked for C++ binding

@Script
class GameEntity {
    int x;
    int y;

    constructor(int posX, int posY) {
        x = posX;
        y = posY;
    }

    public function move(int dx, int dy):void {
        x = x + dx;
        y = y + dy;
    }
}

@Script
class Player extends GameEntity {
    string name;

    constructor(string playerName, int posX, int posY): super(posX, posY) {
        name = playerName;
    }

    public function attack(): void {
        print("Player attacking");
    }
}

// Test execution
Player player = new Player("Hero", 10, 20);
player.move(5, 5);
player.attack();
