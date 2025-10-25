// Comprehensive test for super field access with various scenarios

class Counter {
    protected int count;
    protected bool enabled;

    constructor() {
        count = 0;
        enabled = true;
    }

    public function increment(): void {
        if (enabled) {
            count = count + 1;
        }
    }

    public function getCount(): int {
        return count;
    }
}

class EnhancedCounter extends Counter {
    private int maxCount;

    constructor(int max): super() {
        maxCount = max;
    }

    // Override with super field access
    public function increment(): void {
        if (super.enabled && super.count < maxCount) {
            super.count = super.count + 1;
        }
    }

    // Reset using super fields
    public function reset(): void {
        super.count = 0;
    }

    // Disable counter using super field
    public function disable(): void {
        super.enabled = false;
    }

    // Enable counter using super field
    public function enable(): void {
        super.enabled = true;
    }

    // Get status using super fields
    public function getStatus(): string {
        string status = "Count: " + super.count + "/" + maxCount;
        if (super.enabled) {
            status = status + " (enabled)";
        } else {
            status = status + " (disabled)";
        }
        return status;
    }

    // Batch operations using super fields
    public function addMany(int amount): void {
        int i = 0;
        while (i < amount && super.count < maxCount) {
            if (super.enabled) {
                super.count = super.count + 1;
            }
            i = i + 1;
        }
    }
}

class SmartCounter extends EnhancedCounter {
    private string name;

    constructor(string n, int max): super(max) {
        name = n;
    }

    // Access super fields from grandparent
    public function doubleCount(): void {
        super.count = super.count * 2;
    }

    // Complex operation using super fields
    public function report(): string {
        return name + ": " + super.getStatus();
    }

    // Conditional modification of super fields
    public function smartIncrement(): void {
        if (super.enabled) {
            if (super.count < 5) {
                super.count = super.count + 2;
            } else {
                super.count = super.count + 1;
            }
        }
    }
}

function main(): void {
    print("=== Comprehensive Super Field Access Test ===");

    // Test EnhancedCounter
    print("\n--- EnhancedCounter Test ---");
    EnhancedCounter ec = new EnhancedCounter(10);
    print(ec.getStatus());

    ec.increment();
    ec.increment();
    ec.increment();
    print(ec.getStatus());

    ec.addMany(5);
    print(ec.getStatus());

    ec.disable();
    print(ec.getStatus());
    ec.increment();  // Should not increment when disabled
    print("After trying to increment while disabled: " + ec.getCount());

    ec.enable();
    ec.increment();
    print(ec.getStatus());

    ec.reset();
    print("After reset: " + ec.getStatus());

    // Test SmartCounter
    print("\n--- SmartCounter Test ---");
    SmartCounter sc = new SmartCounter("MyCounter", 20);
    print(sc.report());

    sc.smartIncrement();
    print(sc.report());

    sc.smartIncrement();
    print(sc.report());

    sc.smartIncrement();
    print(sc.report());

    sc.doubleCount();
    print("After doubling: " + sc.report());

    sc.addMany(3);
    print("After adding 3: " + sc.report());

    print("\n=== All tests completed successfully ===");
}

main();
