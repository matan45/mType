// Test: @Script annotation for C++ binding
// Expected: Pass - multiple classes marked with @Script

@Script

class PlayerController {
    private int health;

constructor() {}

    constructor(int initialHealth) {
        health = initialHealth;
    }

    public function update(float dt) : void
 {}

    public function getHealth() : int {
        return health;
    }

    public function takeDamage(int damage) : void {
        health = health - damage;
    }
}

@Script

class GameWorld {
    private int level;

constructor() {}

    constructor(int startLevel) {
        level = startLevel;
    }

public function update(float dt) : void {}

    public function getLevel() : int {
        return level;
    }
}

// Regular class without @Script

class InternalHelper {
    public function help() : void {
        print("Helper");
    }
}

// Test execution
PlayerController player = new PlayerController(100);
GameWorld world = new GameWorld(1);
InternalHelper helper = new InternalHelper();

print("Player health: ");
print(player.getHealth());
print("World level: ");
print(world.getLevel());
helper.help();
