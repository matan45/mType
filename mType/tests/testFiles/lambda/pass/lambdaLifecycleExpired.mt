// Test lambda lifecycle error scenarios
// Tests cases where lambda references might become invalid (weak_ptr expiration)

interface Callback {
    function execute(int value): int;
}

interface AsyncProcessor {
    function processAsync(int value, Callback callback): void;
}

class ProblematicAsyncProcessor implements AsyncProcessor {
    Callback storedCallback;

    public function processAsync(int value, Callback callback): void {
        // Store the callback for later use (potential weak_ptr issue)
        storedCallback = callback;
        print("Stored callback for later execution with value: " + value);
    }

    public function executeStoredCallback(int value): void {
        if (storedCallback != null) {
            int result = storedCallback.execute(value);
            print("Delayed execution result: " + result);
        } else {
            print("ERROR: Stored callback is null or expired!");
        }
    }
}

function createTemporaryCallback(): Callback {
    // This lambda might be cleaned up when function returns
    // depending on the implementation's memory management
    Callback temp = x -> {
        print("Temporary callback executing with: " + x);
        return x * 100;
    };
    return temp;
}

function testCallbackExpiration(): void {
    print("=== Testing callback expiration scenarios ===");

    ProblematicAsyncProcessor processor = new ProblematicAsyncProcessor();

    // Test 1: Immediate callback (should work)
    print("Test 1: Immediate callback");
    Callback immediate = x -> x + 50;
    processor.processAsync(10, immediate);
    processor.executeStoredCallback(10);

    // Test 2: Temporary callback from function
    print("Test 2: Temporary callback from function");
    Callback temporary = createTemporaryCallback();
    processor.processAsync(20, temporary);
    processor.executeStoredCallback(20);

    // Test 3: Callback from nested scope
    print("Test 3: Callback from nested scope");
    if (true) {
        Callback nested = x -> {
            print("Nested callback with: " + x);
            return x / 2;
        };
        processor.processAsync(30, nested);
        // nested goes out of scope here
    }
    // Try to execute after scope exit
    processor.executeStoredCallback(30);

    // Test 4: Multiple rapid callback changes
    print("Test 4: Rapid callback replacement");
    for (int i = 0; i < 5; i++) {
        int factor = i + 1;
        Callback rapid = x -> x * factor;
        processor.processAsync(i * 10, rapid);
        // Previous callback may be replaced/expired
    }
    processor.executeStoredCallback(40);
}

function main(): void {
    print("Testing lambda lifecycle error scenarios...");

    // Test expired/invalid callback scenarios
    testCallbackExpiration();

    // Test massive callback creation and cleanup
    print("=== Testing massive callback cleanup ===");
    Callback[] massiveArray = new Callback[100];
    for (int i = 0; i < 100; i++) {
        int multiplier = i;
        massiveArray[i] = x -> x * multiplier;
    }

    // Use only a few
    Callback cb1 = massiveArray[10];
    Callback cb2 = massiveArray[50];
    Callback cb3 = massiveArray[90];
    int result1 = cb1.execute(5);
    int result2 = cb2.execute(5);
    int result3 = cb3.execute(5);

    print("Massive array results: " + result1 + ", " + result2 + ", " + result3);

    // Clear array to test cleanup
    for (int i = 0; i < 100; i++) {
        massiveArray[i] = null;
    }

    print("Lambda lifecycle test completed!");
}

main();