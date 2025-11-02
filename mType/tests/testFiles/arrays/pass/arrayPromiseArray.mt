// Test arrays of promise-like objects
print("Testing arrays of promise-like objects");

class Promise {
    int value;
    bool resolved;

    constructor(int v) {
        value = v;
        resolved = false;
    }

    public function resolve(): void {
        resolved = true;
    }

    public function isResolved(): bool {
        return resolved;
    }

    public function getValue(): int {
        return value;
    }
}

Promise[] promises = new Promise[4];
promises[0] = new Promise(10);
promises[1] = new Promise(20);
promises[2] = new Promise(30);
promises[3] = new Promise(40);

print("Created 4 promises");

// Resolve some promises
promises[0].resolve();
promises[2].resolve();

print("Resolved promises 0 and 2");

// Check which promises are resolved
for (int i = 0; i < promises.length; i++) {
    if (promises[i].isResolved()) {
        print("  Promise " + i + " resolved with value: " + promises[i].getValue());
    } else {
        print("  Promise " + i + " pending");
    }
}

// Resolve all
for (int i = 0; i < promises.length; i++) {
    promises[i].resolve();
}

print("All promises resolved:");
for (int i = 0; i < promises.length; i++) {
    print("  Value " + i + ": " + promises[i].getValue());
}

print("Promise array test completed");
