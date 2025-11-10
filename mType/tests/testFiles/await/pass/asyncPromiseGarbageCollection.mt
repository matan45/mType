// Test promise garbage collection and cleanup

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Promise Garbage Collection Test ===");

class Resource {
    int id;

    public constructor(int resourceId) {
        this.id = resourceId;
        print("Resource created: " + this.id);
    }

    public function getId(): int {
        return this.id;
    }
}

function async createAndReleaseResource(int id): Promise<Int> {
    Resource res = new Resource(id);
    print("Using resource: " + res.getId());
    // Resource should be eligible for GC after this function returns
    return new Int(res.getId());
}

function async testGarbageCollection(): Promise<Int> {
    print("Creating multiple resources...");

    // Create resources that should be cleaned up
    Int r1 = await createAndReleaseResource(1);
    Int r2 = await createAndReleaseResource(2);
    Int r3 = await createAndReleaseResource(3);

    int total = r1.getValue() + r2.getValue() + r3.getValue();
    print("Total: " + total);

    return new Int(total);
}

function async main(): Promise<Int> {
    Int result = await testGarbageCollection();
    print("Final result: " + result);
    return result;
}

main();
