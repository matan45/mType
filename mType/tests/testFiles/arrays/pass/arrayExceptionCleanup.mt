// Test resource cleanup on exception
print("Testing exception cleanup");

class SafeResource {
    int id;

    constructor(int i) {
        id = i;
    }

    public function getId(): int {
        return id;
    }
}

SafeResource[] resources = new SafeResource[5];
for (int i = 0; i < resources.length; i++) {
    resources[i] = new SafeResource(i);
}

print("Resources allocated:");
for (int i = 0; i < resources.length; i++) {
    print("  Resource " + resources[i].getId());
}

// Simulate exception scenario with early return
function processArray(SafeResource[] arr): void {
    SafeResource[] temp = new SafeResource[10];
    for (int i = 0; i < 5; i++) {
        temp[i] = new SafeResource(i * 10);
    }
    // Early return simulates exception
    return;
}

processArray(resources);

print("After exception simulation:");
print("Original array still valid:");
for (int i = 0; i < resources.length; i++) {
    print("  Resource " + resources[i].getId());
}

print("Exception cleanup test completed");
