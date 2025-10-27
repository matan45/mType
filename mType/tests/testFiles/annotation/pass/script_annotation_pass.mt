// Test: @Script annotation on classes
// Expected: Pass - classes are marked for C++ binding

@Script
class GameEntity {
    Int x;
    Int y;

    GameEntity(Int posX, Int posY) {
        x = posX;
        y = posY;
    }

    void move(Int dx, Int dy) {
        x = x + dx;
        y = y + dy;
    }
}

@Script
class Player extends GameEntity {
    String name;

    Player(String playerName, Int posX, Int posY) {
        super(posX, posY);
        name = playerName;
    }

    void attack() {
        print("Player attacking");
    }
}

// Test execution
Player player = new Player("Hero", 10, 20);
player.move(5, 5);
player.attack();
