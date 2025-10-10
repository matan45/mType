// Test exception object lifecycle and memory management
import "../../lib/exceptions/Exception.mt";

// Custom exception with destructor tracking
class TrackedException extends Exception {
    public static int instanceCount = 0;
    public int id;

    constructor(string msg, int identifier): super(msg) {
        this.id = identifier;
        TrackedException::instanceCount = TrackedException::instanceCount + 1;
        print("TrackedException " + identifier + " created (count: " + TrackedException::instanceCount + ")");
    }

    public function getId(): int {
        return this.id;
    }
}

// Test 1: Exception object lifecycle in try-catch
function testBasicExceptionLifecycle(): void {
    print("=== Test 1: Basic exception lifecycle ===");
    int initialCount = TrackedException::instanceCount;

    try {
        TrackedException e = new TrackedException("Test exception", 1);
        throw e;
    } catch (TrackedException e) {
        print("Caught exception " + e.getId());
        // Exception is still in scope here
    }
    // Exception should be out of scope now

    print("After catch block");
}

// Test 2: Multiple exceptions created but only one thrown
function testMultipleExceptionCreation(): void {
    print("\n=== Test 2: Multiple exception creation ===");
    int initialCount = TrackedException::instanceCount;

    try {
        TrackedException e1 = new TrackedException("Exception 1", 10);
        TrackedException e2 = new TrackedException("Exception 2", 11);
        TrackedException e3 = new TrackedException("Exception 3", 12);

        // Only throw one
        throw e2;
    } catch (TrackedException e) {
        print("Caught exception " + e.getId());
    }

    print("After catch: e1, e2, e3 should be cleaned up");
}

// Test 3: Exception re-throw doesn't create new object
function testRethrowSameObject(): void {
    print("\n=== Test 3: Re-throw same object ===");
    int countBeforeTry = TrackedException::instanceCount;

    try {
        try {
            TrackedException e = new TrackedException("Rethrow test", 20);
            print("Inner try: created 1 exception");
            throw e;
        } catch (TrackedException e) {
            print("Inner catch: re-throwing (no new object created)");
            throw e;  // Re-throw same object
        }
    } catch (TrackedException e) {
        print("Outer catch: caught exception " + e.getId());
    }

    int countAfterCatch = TrackedException::instanceCount;
    int created = countAfterCatch - countBeforeTry;
    print("Total new exceptions created: " + created + " (should be 1)");
}

// Test 4: Exception in loop - many objects created and cleaned
function testExceptionsInLoop(): void {
    print("\n=== Test 4: Exceptions in loop ===");
    int countBefore = TrackedException::instanceCount;

    int i = 0;
    while (i < 5) {
        try {
            TrackedException e = new TrackedException("Loop exception", 30 + i);
            throw e;
        } catch (TrackedException e) {
            print("Iteration " + i + ": caught exception " + e.getId());
        }
        i = i + 1;
    }

    int countAfter = TrackedException::instanceCount;
    int created = countAfter - countBefore;
    print("Total exceptions created in loop: " + created + " (should be 5)");
}

// Test 5: Exception with large data - memory pressure test
class LargeDataException extends Exception {
    public string data;

    constructor(string msg, int size): super(msg) {
        // Create a large string by concatenation
        this.data = "";
        int i = 0;
        while (i < size) {
            this.data = this.data + "X";
            i = i + 1;
        }
    }

    public function getDataSize(): int {
        // String doesn't have length() method, so we return the size we created
        return 0;  // Placeholder
    }
}

function testLargeExceptionData(): void {
    print("\n=== Test 5: Large exception data ===");

    int i = 0;
    while (i < 10) {
        try {
            // Create exception with increasingly large data
            LargeDataException e = new LargeDataException("Large data exception", 100);
            throw e;
        } catch (LargeDataException e) {
            print("Iteration " + i + ": caught large exception");
            // Exception should be cleaned up when catch scope exits
        }
        i = i + 1;
    }

    print("Completed large exception test (should not leak memory)");
}

// Test 6: Nested try-catch with multiple exception objects
function testNestedMultipleExceptions(): void {
    print("\n=== Test 6: Nested try-catch with multiple exceptions ===");
    int countBefore = TrackedException::instanceCount;

    try {
        TrackedException e1 = new TrackedException("Outer exception", 40);

        try {
            TrackedException e2 = new TrackedException("Middle exception", 41);

            try {
                TrackedException e3 = new TrackedException("Inner exception", 42);
                throw e3;
            } catch (TrackedException e) {
                print("Inner catch: " + e.getId());
                throw e2;  // Throw different exception
            }
        } catch (TrackedException e) {
            print("Middle catch: " + e.getId());
            throw e1;  // Throw different exception
        }
    } catch (TrackedException e) {
        print("Outer catch: " + e.getId());
    }

    int countAfter = TrackedException::instanceCount;
    int created = countAfter - countBefore;
    print("Total exceptions created: " + created + " (should be 3)");
}

// Test 7: Exception passed as function parameter
function processException(TrackedException e): string {
    return "Processed exception " + e.getId();
}

function testExceptionAsParameter(): void {
    print("\n=== Test 7: Exception as function parameter ===");
    int countBefore = TrackedException::instanceCount;

    try {
        TrackedException e = new TrackedException("Parameter test", 50);
        string result = processException(e);
        print(result);
        throw e;
    } catch (TrackedException e) {
        print("Caught exception " + e.getId());
    }

    int countAfter = TrackedException::instanceCount;
    int created = countAfter - countBefore;
    print("Total exceptions created: " + created + " (should be 1)");
}

// Run all tests
testBasicExceptionLifecycle();
testMultipleExceptionCreation();
testRethrowSameObject();
testExceptionsInLoop();
testLargeExceptionData();
testNestedMultipleExceptions();
testExceptionAsParameter();

print("\n=== Summary ===");
print("Total TrackedException instances created: " + TrackedException::instanceCount);
print("All exception lifecycle tests completed");
