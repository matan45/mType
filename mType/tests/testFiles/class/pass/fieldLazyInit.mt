// Test: Lazy field initialization pattern
// Expected: Pass - demonstrates lazy initialization

class ExpensiveResource {
    private string name;

    public constructor(string name) {
        this.name = name;
        print("ExpensiveResource created: " + name);
    }

    public function getName(): string {
        return this.name;
    }
}

class LazyContainer {
    private ExpensiveResource resource;
    private bool initialized;

    public constructor() {
        this.initialized = false;
        print("LazyContainer created (resource not initialized)");
    }

    public function getResource(): ExpensiveResource {
        if (!this.initialized) {
            print("Lazy initialization triggered");
            this.resource = new ExpensiveResource("LazyResource");
            this.initialized = true;
        }
        return this.resource;
    }

    public function isInitialized(): bool {
        return this.initialized;
    }
}

// Test lazy initialization
print("Test 1: Create container");
LazyContainer c1 = new LazyContainer();
print("Initialized: " + c1.isInitialized());

print("\nTest 2: First access");
ExpensiveResource r1 = c1.getResource();
print("Resource name: " + r1.getName());
print("Initialized: " + c1.isInitialized());

print("\nTest 3: Second access");
ExpensiveResource r2 = c1.getResource();
print("Resource name: " + r2.getName());
print("Same instance: " + (r1 == r2));

print("\nTest 4: New container");
LazyContainer c2 = new LazyContainer();
ExpensiveResource r3 = c2.getResource();
print("Different instance: " + (r1 != r3));
