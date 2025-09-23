// Complex integration: Classes + Final variables (namespaces removed)
final int MAX_ENTITIES = 1000;
final string VERSION = "2.0";

class Entity {
    final int id;
    string name;
    static int nextId = 1;
    
    constructor(string entityName) {
        id = nextId;
        nextId = nextId + 1;
        name = entityName;
    }
    
    function getId(): int {
        return id;
    }
    
    static function getNextId(): int {
        return nextId;
    }
}

final float GRAVITY = 9.8;

class Vector3 {
    float x;
    float y;
    float z;
    
    constructor(float x_val, float y_val, float z_val) {
        x = x_val;
        y = y_val;
        z = z_val;
    }
    
    function magnitude(): float {
        return x * x + y * y + z * z; // Simplified
    }
}

function applyGravity(Entity entity): string {
    return "Applying gravity " + GRAVITY + " to " + entity.name;
}

final int MAX_VERTICES = 10000;
int frameCount = 0;

function renderEntity(Entity entity): int {
    frameCount = frameCount + 1;
    return frameCount;
}

class Calculator {
    function fibonacci(int n): int {
        if (n <= 1) return n;
        return fibonacci(n - 1) + fibonacci(n - 2);
    }
}

Calculator calc = new Calculator();
print("Test - Fibonacci(6): " + calc.fibonacci(6));

// Test complex interactions
Entity player = new Entity("Player");
Entity enemy = new Entity("Enemy");

print(player.getId());
print(enemy.getId());
print(Entity::getNextId());

Vector3 position = new Vector3(1.0, 2.0, 3.0);
print(position.magnitude());

int frame1 = renderEntity(player);
int frame2 = renderEntity(enemy);
print(frame1);
print(frame2);

// Direct usage without namespaces
Entity npc = new Entity("NPC");
Vector3 velocity = new Vector3(0.5, -1.0, 0.0);
int frame3 = renderEntity(npc);

print(npc.getId());
print(velocity.magnitude());
print(frame3);
print("test passed");