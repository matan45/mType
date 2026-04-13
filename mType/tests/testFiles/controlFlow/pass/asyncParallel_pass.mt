// Test parallel async operations
// Validates multiple concurrent promises created and awaited

class AsyncResult {
    int id;
    int value;

    public constructor(int resultId, int resultValue) {
        this.id = resultId;
        this.value = resultValue;
    }

    public function getId(): int {
        return this.id;
    }

    public function getValue(): int {
        return this.value;
    }
}

print("=== Parallel Async Operations Test ===");

// Three independent async operations
function async operation1(): Promise<AsyncResult> {
    print("Starting operation 1");
    AsyncResult r = new AsyncResult(1, 100);
    return r;
}

function async operation2(): Promise<AsyncResult> {
    print("Starting operation 2");
    AsyncResult r = new AsyncResult(2, 200);
    return r;
}

function async operation3(): Promise<AsyncResult> {
    print("Starting operation 3");
    AsyncResult r = new AsyncResult(3, 300);
    return r;
}

// Test parallel promise creation
function async runParallel(): Promise<void> {
    print("Creating all promises in parallel");

    // Create all promises (they start executing)
    Promise<AsyncResult> p1 = operation1();
    Promise<AsyncResult> p2 = operation2();
    Promise<AsyncResult> p3 = operation3();

    print("All promises created, now awaiting results");

    // Await results in order
    AsyncResult r1 = await p1;
    print("Operation " + r1.getId() + " completed with value " + r1.getValue());

    AsyncResult r2 = await p2;
    print("Operation " + r2.getId() + " completed with value " + r2.getValue());

    AsyncResult r3 = await p3;
    print("Operation " + r3.getId() + " completed with value " + r3.getValue());

    // Calculate total
    int total = r1.getValue() + r2.getValue() + r3.getValue();
    print("Total from parallel operations: " + total);

}

// Test sequential vs parallel patterns
function async comparePatterns(): Promise<void> {
    print("");
    print("Testing sequential pattern:");

    AsyncResult s1 = await operation1();
    AsyncResult s2 = await operation2();
    AsyncResult s3 = await operation3();

    int sequentialTotal = s1.getValue() + s2.getValue() + s3.getValue();
    print("Sequential total: " + sequentialTotal);

}

// Main function
function async main(): Promise<void> {
    runParallel();
    comparePatterns();

    print("");
    print("Parallel operations test complete");
}

main();
