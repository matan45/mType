// Test lambda circular reference error
// This test demonstrates the error when a lambda tries to reference
// a variable that hasn't been declared yet (forward reference)

interface Callback {
    function execute(int value): int;
}

function testCircularReferences(): void {
    print("=== Testing potential circular references ===");

	Callback lambda2 = x -> {
        print("Lambda2 processing: " + x);
        if (x > 0 && lambda1 != null) {
            return lambda1.execute(x - 1);
        }
        return x;
    };

    // This pattern might create circular references between lambdas
    Callback lambda1 = x -> {
        print("Lambda1 processing: " + x);
        if (x > 0 && lambda2 != null) {
            return lambda2.execute(x - 1);
        }
        return x;
    };


    // This might create a circular dependency
    int result = lambda1.execute(3);
    print("Circular reference test result: " + result);

    // Clear references to break potential cycles
    lambda1 = null;
    lambda2 = null;
}

function main(): void {
    print("Testing lambda circular reference error...");
    testCircularReferences();
}

main();
