// Test basic lambda lifecycle scenarios
// Tests lambda creation, execution, and cleanup

interface Processor {
    function process(int value): int;
}

interface AdvancedProcessor {
    function processWithCallback(int value, Processor callback): int;
}

class ProcessorManager implements AdvancedProcessor {
    function processWithCallback(int value, Processor callback): int {
        print("Processing value: " + value);
        int result = callback.process(value);
        print("Callback returned: " + result);
        return result;
    }
}

function main(): void {
    print("Testing basic lambda lifecycle...");

    ProcessorManager manager = new ProcessorManager();

    // Test 1: Simple lambda creation and immediate use
    print("=== Test 1: Immediate lambda use ===");
    int result1 = manager.processWithCallback(10, x -> x * 2);
    print("Result 1: " + result1);

    // Test 2: Lambda stored in variable
    print("=== Test 2: Lambda in variable ===");
    Processor doubler = x -> x * 2;
    int result2 = manager.processWithCallback(15, doubler);
    print("Result 2: " + result2);

    // Test 3: Multiple lambda instances
    print("=== Test 3: Multiple lambdas ===");
    Processor adder = x -> x + 100;
    Processor multiplier = x -> x * 3;

    int result3a = manager.processWithCallback(5, adder);
    int result3b = manager.processWithCallback(5, multiplier);
    print("Result 3a: " + result3a);
    print("Result 3b: " + result3b);

    // Test 4: Nested lambda calls
    print("=== Test 4: Nested lambda processing ===");
    Processor complexProcessor = x -> {
        int temp = x + 10;
        return temp * 2;
    };

    int result4 = manager.processWithCallback(7, complexProcessor);
    print("Result 4: " + result4);

    print("Basic lambda lifecycle test completed!");
}

main();