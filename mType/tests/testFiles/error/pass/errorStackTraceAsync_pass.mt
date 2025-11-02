// Test: Stack trace across await boundaries
// Expected: Stack traces should be preserved across async/await boundaries
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/exceptions/RuntimeException.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";

// Test 1: Exception in async function
function async throwAsync(): Promise<String> {
    print("Async function throwing");
    RuntimeException e = new RuntimeException("Async error");
    throw e;
    return new String("never");
}

function async testAsyncException(): Promise<void> {
    print("=== Test 1: Async exception ===");
    try {
        Promise<String> p = throwAsync();
        String result = await p;
        print("Result: " + result.toString());
    } catch (RuntimeException e) {
        print("Caught async exception: " + e.getMessage());
        string trace = e.getStackTrace();
        if (trace != "") {
            print("Stack trace from async: true");
        }
        print("Async exception handled");
    }
    return null;
}

// Test 2: Multiple awaits with exception in middle
function async firstOperation(): Promise<Int> {
    print("First operation executing");
    return new Int(100);
}

function async secondOperation(): Promise<Int> {
    print("Second operation throwing");
    RuntimeException e = new RuntimeException("Second operation failed");
    throw e;
    return new Int(200);
}

function async thirdOperation(): Promise<Int> {
    print("Third operation executing");
    return new Int(300);
}

function async testMultipleAwaits(): Promise<void> {
    print("=== Test 2: Multiple awaits with exception ===");
    try {
        Promise<Int> p1 = firstOperation();
        Int r1 = await p1;
        print("First result: " + r1.toString());

        Promise<Int> p2 = secondOperation();
        Int r2 = await p2;
        print("Second result: " + r2.toString());

        Promise<Int> p3 = thirdOperation();
        Int r3 = await p3;
        print("Third result: " + r3.toString());
    } catch (RuntimeException e) {
        print("Caught during awaits: " + e.getMessage());
        string trace = e.getStackTrace();
        if (trace != "") {
            print("Stack trace across awaits: true");
        }
        print("Multiple await exception handled");
    }
    return null;
}

// Test 3: Nested async calls with exception
function async deepAsync(): Promise<String> {
    print("Deep async function");
    RuntimeException e = new RuntimeException("Deep async error");
    throw e;
    return new String("deep");
}

function async middleAsync(): Promise<String> {
    print("Middle async function");
    Promise<String> p = deepAsync();
    String result = await p;
    return new String("middle: " + result.toString());
}

function async topAsync(): Promise<void> {
    print("=== Test 3: Nested async exception ===");
    try {
        Promise<String> p = middleAsync();
        String result = await p;
        print("Top result: " + result.toString());
    } catch (RuntimeException e) {
        print("Caught nested async: " + e.getMessage());
        string trace = e.getStackTrace();
        if (trace != "") {
            print("Stack trace includes nested async: true");
        }
        print("Nested async exception handled");
    }
    return null;
}

// Test 4: Await in loop with exception
function async processItem(Int index): Promise<Int> {
    print("Processing item: " + index.toString());
    if (index.toInt() == 2) {
        RuntimeException e = new RuntimeException("Item 2 failed");
        throw e;
    }
    return new Int(index.toInt() * 10);
}

function async testAwaitLoop(): Promise<void> {
    print("=== Test 4: Await in loop ===");
    try {
        Int i = new Int(0);
        while (i.toInt() < 4) {
            print("Loop iteration: " + i.toString());
            Promise<Int> p = processItem(i);
            Int result = await p;
            print("Processed: " + result.toString());
            i = new Int(i.toInt() + 1);
        }
        print("Loop completed");
    } catch (RuntimeException e) {
        print("Caught in loop: " + e.getMessage());
        string trace = e.getStackTrace();
        if (trace != "") {
            print("Stack trace from loop await: true");
        }
        print("Loop exception handled");
    }
    return null;
}

// Test 5: Async method with exception
class AsyncService {
    public function async fetchData(Int id): Promise<String> {
        print("Fetching data for ID: " + id.toString());
        if (id.toInt() < 0) {
            RuntimeException e = new RuntimeException("Invalid ID");
            throw e;
        }
        return new String("Data_" + id.toString());
    }

    public function async processData(Int id): Promise<String> {
        print("Processing data");
        Promise<String> p = fetchData(id);
        String data = await p;
        return new String("Processed: " + data.toString());
    }
}

function async testAsyncMethod(): Promise<void> {
    print("=== Test 5: Async method exception ===");
    AsyncService service = new AsyncService();

    try {
        Promise<String> p = service.processData(new Int(-1));
        String result = await p;
        print("Result: " + result.toString());
    } catch (RuntimeException e) {
        print("Caught async method error: " + e.getMessage());
        string trace = e.getStackTrace();
        if (trace != "") {
            print("Stack trace from async method: true");
        }
        print("Async method exception handled");
    }
    return null;
}

// Test 6: Await with conditional and exception
function async conditionalAsync(bool shouldFail): Promise<Int> {
    print("Conditional async: shouldFail=" + shouldFail);
    if (shouldFail) {
        RuntimeException e = new RuntimeException("Conditional failure");
        throw e;
    }
    return new Int(42);
}

function async testConditionalAwait(): Promise<void> {
    print("=== Test 6: Conditional await exception ===");

    // Test success case
    try {
        print("Testing success case");
        Promise<Int> p1 = conditionalAsync(false);
        Int result1 = await p1;
        print("Success result: " + result1.toString());
    } catch (RuntimeException e) {
        print("Unexpected error: " + e.getMessage());
    }

    // Test failure case
    try {
        print("Testing failure case");
        Promise<Int> p2 = conditionalAsync(true);
        Int result2 = await p2;
        print("Failure result: " + result2.toString());
    } catch (RuntimeException e) {
        print("Caught conditional error: " + e.getMessage());
        string trace = e.getStackTrace();
        if (trace != "") {
            print("Stack trace from conditional: true");
        }
        print("Conditional exception handled");
    }
    return null;
}

// Test 7: Await with finally and exception
function async throwingAsync(): Promise<void> {
    print("Function about to throw");
    RuntimeException e = new RuntimeException("Finally test error");
    throw e;
    return null;
}

function async testAwaitFinally(): Promise<void> {
    print("=== Test 7: Await with finally ===");
    bool finallyCalled = false;

    try {
        Promise<void> p = throwingAsync();
        await p;
        print("Should not reach here");
    } catch (RuntimeException e) {
        print("Caught in finally test: " + e.getMessage());
        string trace = e.getStackTrace();
        if (trace != "") {
            print("Stack trace available: true");
        }
    } finally {
        print("Finally block executed");
        finallyCalled = true;
    }

    if (finallyCalled) {
        print("Finally was called: true");
    }
    print("Finally test completed");
    return null;
}

// Main async entry point
function async main(): Promise<void> {
    Promise<void> p1 = testAsyncException();
    await p1;
    print("");

    Promise<void> p2 = testMultipleAwaits();
    await p2;
    print("");

    Promise<void> p3 = topAsync();
    await p3;
    print("");

    Promise<void> p4 = testAwaitLoop();
    await p4;
    print("");

    Promise<void> p5 = testAsyncMethod();
    await p5;
    print("");

    Promise<void> p6 = testConditionalAwait();
    await p6;
    print("");

    Promise<void> p7 = testAwaitFinally();
    await p7;
    print("");

    print("Async stack trace test completed");
    return null;
}

main();
