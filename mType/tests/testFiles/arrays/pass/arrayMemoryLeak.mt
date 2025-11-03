// Test memory leak detection (arrays should be properly cleaned)
print("Testing memory leak prevention");

class Resource {
    int id;

    constructor(int i) {
        id = i;
    }

    public function getId(): int {
        return id;
    }
}

// Create many arrays in a loop
int iterations = 100;
for (int i = 0; i < iterations; i++) {
    Resource[] temp = new Resource[10];
    for (int j = 0; j < temp.length; j++) {
        temp[j] = new Resource(i * 10 + j);
    }
    // Array goes out of scope and should be collected
}

print("Created and disposed " + iterations + " arrays");

// Final allocation should still work
Resource[] finalResource = new Resource[5];
for (int i = 0; i < finalResource.length; i++) {
    finalResource[i] = new Resource(i);
}

print("Final array allocated successfully:");
for (int i = 0; i < finalResource.length; i++) {
    print("  Resource " + finalResource[i].getId());
}

print("Memory leak test completed");
