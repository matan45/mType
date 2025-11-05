// Lambda block without explicit return for void test
interface VoidFunction {
    function execute(int x) : void;
}

class Printer {
    int callCount;

    constructor() {
        this.callCount = 0;
    }

    public function increment() {
        this.callCount = this.callCount + 1;
    }

    public function getCount() : int {
        return this.callCount;
    }
}

print("=== Implicit Void Return Test ===");

Printer p = new Printer();

VoidFunction counter = x -> {
    for (int i = 0; i < x; i = i + 1) {
        p.increment();
    }
    // No explicit return needed for void
};

counter.execute(5);
print("Count after 5: " + p.getCount());

counter.execute(3);
print("Count after 3 more: " + p.getCount());

// Void lambda with multiple paths
int[] data = [0, 0, 0];
VoidFunction updater = idx -> {
    if (idx >= 0 && idx < data.length) {
        data[idx] = data[idx] + 1;
    } else {
        print("Invalid index: " + idx);
    }
    // No return in either branch
};

updater.execute(0);
updater.execute(1);
updater.execute(5);  // Invalid

print("data[0] = " + data[0]);
print("data[1] = " + data[1]);
print("data[2] = " + data[2]);

print("Implicit void return complete");
