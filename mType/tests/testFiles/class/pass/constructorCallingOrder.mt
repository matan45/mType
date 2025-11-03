// Test: Constructor calling order in inheritance hierarchy
// Expected: Pass - demonstrates constructor execution order

class Level1 {
    protected int level;

    public constructor() {
        this.level = 1;
        print("Level1 constructor start");
        this.initialize();
        print("Level1 constructor end");
    }

    public function initialize(): void {
        print("Level1 initialize");
    }
}

class Level2 extends Level1 {
    protected string name;

    public constructor(string name) : super() {
        print("Level2 constructor start");
        this.name = name;
        this.level = 2;
        this.initialize();
        print("Level2 constructor end");
    }

    public function initialize(): void {
        print("Level2 initialize for " + this.name);
    }
}

class Level3 extends Level2 {
    protected int value;

    public constructor(string name, int value) : super(name) {
        print("Level3 constructor start");
        this.value = value;
        this.level = 3;
        this.initialize();
        print("Level3 constructor end");
    }

    public function initialize(): void {
        print("Level3 initialize with value " + this.value);
    }

    public function display(): void {
        print("Level: " + this.level + ", Name: " + this.name + ", Value: " + this.value);
    }
}

// Test constructor calling order
print("Creating Level3 object:");
Level3 obj = new Level3("Test", 42);
print("\nFinal state:");
obj.display();
