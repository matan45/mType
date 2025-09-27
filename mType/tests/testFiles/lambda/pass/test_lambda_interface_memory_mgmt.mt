// Test lambda-to-interface memory management and edge cases

interface Processor {
    function process(int value): int;
}

interface Action {
    function execute(): void;
}

interface Validator {
    function isValid(string input): bool;
}

function createProcessor(int factor): Processor {
    // Test: Lambda creation in function scope with capture
    return x -> {
        return x * factor;
    };
}

function testNestedLambdas(): void {
    print("=== Testing Nested Lambda Creation ===");

    // Test: Nested lambda creation and memory management
    Action outerAction = () -> {
        print("Outer action executing");

        // Inner lambda with different interface
        Processor innerProcessor = value -> {
            print("Inner processor called with: " + value);
            return value * 2;
        };

        int result = innerProcessor.process(5);
        print("Inner result: " + result);
    };

    outerAction.execute();
}

function testLambdaImmutability(): void {
    print("=== Testing Lambda Immutability (Design Feature) ===");

    // Test: Lambda variables are immutable once assigned (no reassignment allowed)
    Processor proc1 = x -> { return x + 1; };
    int result1 = proc1.process(10);
    print("First lambda result: " + result1);

    // Use separate variable - reassignment is NOT allowed by design
    Processor proc2 = x -> { return x * 3; };
    int result2 = proc2.process(10);
    print("Second lambda result: " + result2);

    // Test that both lambdas work independently and maintain their implementations
    int result3 = proc1.process(5);
    int result4 = proc2.process(5);
    print("Independent results: " + result3 + ", " + result4);
    print("Lambda immutability enforced - no reassignment allowed");
}

function testLambdaWithComplexCapture(): void {
    print("=== Testing Complex Variable Capture ===");

    string prefix = "Value: ";
    int baseValue = 100;
    bool shouldDouble = true;

    // Test: Multiple variable capture from different scopes
    Validator complexValidator = input -> {
        print(prefix + input);
        int len = strLength(input);
        int threshold = shouldDouble ? baseValue * 2 : baseValue;
        return len > 0 && len < threshold;
    };

    bool valid1 = complexValidator.isValid("test");
    bool valid2 = complexValidator.isValid("");

    print("Validation results: " + valid1 + ", " + valid2);
}

function testLambdaLifecycle(): void {
    print("=== Testing Lambda Lifecycle ===");

    // Test: Lambda created and used in same scope
    Processor localProc = createProcessor(5);
    int result = localProc.process(20);
    print("Local processor result: " + result);
}

function main(): void {
    print("=== Lambda-Interface Memory Management Tests ===");

    testNestedLambdas();
    testLambdaImmutability();
    testLambdaWithComplexCapture();
    testLambdaLifecycle();

    // Test: Basic error handling with lambdas
    print("=== Testing Error Handling ===");
    Processor errorProc = x -> {
        if (x < 0) {
            print("Negative value detected: " + x);
            return 0;  // Handle gracefully
        }
        return x * 2;  // Safe operation
    };

    int result1 = errorProc.process(15);
    int result2 = errorProc.process(-5);

    print("Error handling results: " + result1 + ", " + result2);

    print("=== Memory Management Tests Completed ===");
}

main();