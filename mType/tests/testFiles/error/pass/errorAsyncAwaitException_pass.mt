// Test: Exception from awaited promise
// Expected: Exceptions thrown in async functions are caught when awaited
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/exceptions/RuntimeException.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";

class AsyncOperationException extends Exception {
    public Int operationId;

    public constructor(string msg, Int id): super(msg) {
        operationId = id;
    }

    public function getId(): Int {
        return operationId;
    }
}

// Test 1: Simple await exception
function async throwOnAwait(): Promise<String> {
    print("Async operation starting");
    Exception e = new Exception("Await error");
    throw e;
    return new String("never");
}

function async testSimpleAwait(): Promise<String> {
    try {
        print("Awaiting throwing function");
        Promise<String> p = throwOnAwait();
        String result = await p;
        print("Should not reach: " + result.toString());
        return new String("unexpected");
    } catch (Exception e) {
        print("Caught from await: " + e.getMessage());
        return new String("caught");
    }
}

// Test 2: Multiple awaits with exceptions
function async operation1(): Promise<Int> {
    print("Operation 1 executing");
    return new Int(10);
}

function async operation2(): Promise<Int> {
    print("Operation 2 throwing");
    RuntimeException e = new RuntimeException("Operation 2 failed");
    throw e;
    return new Int(20);
}

function async operation3(): Promise<Int> {
    print("Operation 3 executing");
    return new Int(30);
}

function async testMultipleAwaits(): Promise<String> {
    try {
        print("Starting multiple operations");

        Promise<Int> p1 = operation1();
        Int r1 = await p1;
        print("Operation 1 result: " + r1.toString());

        Promise<Int> p2 = operation2();
        Int r2 = await p2;
        print("Operation 2 result: " + r2.toString());

        Promise<Int> p3 = operation3();
        Int r3 = await p3;
        print("Operation 3 result: " + r3.toString());

        return new String("all completed");
    } catch (RuntimeException e) {
        print("Caught during operations: " + e.getMessage());
        return new String("operation failed");
    }
}

// Test 3: Await in different contexts
function async throwingOperation(Int id): Promise<Int> {
    print("Operation " + id.toString() + " starting");
    AsyncOperationException e = new AsyncOperationException("Operation failed", id);
    throw e;
    return new Int(0);
}

function async awaitInConditional(bool shouldFail): Promise<String> {
    try {
        if (shouldFail) {
            print("Conditional branch - will fail");
            Promise<Int> p = throwingOperation(new Int(1));
            Int result = await p;
            return new String("failed: " + result.toString());
        }
        print("Conditional branch - success");
        return new String("success");
    } catch (AsyncOperationException e) {
        print("Caught in conditional: " + e.getMessage());
        print("Failed operation ID: " + e.getId().toString());
        return new String("handled");
    }
}

// Test 4: Await in loop
function async awaitInLoop(): Promise<String> {
    try {
        Int i = new Int(0);
        while (i.getValue() < 3) {
            print("Loop iteration: " + i.toString());

            if (i.getValue() == 1) {
                Promise<Int> p = throwingOperation(i);
                Int result = await p;
                print("Never reached: " + result.toString());
            }

            i = new Int(i.getValue() + 1);
        }
        return new String("loop completed");
    } catch (AsyncOperationException e) {
        print("Caught in loop: " + e.getMessage());
        print("At operation: " + e.getId().toString());
        return new String("loop interrupted");
    }
}

// Test 5: Sequential awaits with mixed success/failure
function async successOperation(): Promise<String> {
    print("Success operation");
    return new String("OK");
}

function async failOperation(): Promise<String> {
    print("Fail operation");
    Exception e = new Exception("Failed operation");
    throw e;
    return new String("FAIL");
}

function async sequentialAwaits(): Promise<String> {
    try {
        print("Sequential operations start");

        Promise<String> p1 = successOperation();
        String r1 = await p1;
        print("First result: " + r1.toString());

        Promise<String> p2 = successOperation();
        String r2 = await p2;
        print("Second result: " + r2.toString());

        Promise<String> p3 = failOperation();
        String r3 = await p3;
        print("Third result: " + r3.toString());

        Promise<String> p4 = successOperation();
        String r4 = await p4;
        print("Fourth result: " + r4.toString());

        return new String("all done");
    } catch (Exception e) {
        print("Sequential caught: " + e.getMessage());
        return new String("sequence stopped");
    }
}

// Test 6: Nested awaits with exceptions
function async innerAwait(): Promise<Int> {
    print("Inner await throwing");
    Exception e = new Exception("Inner error");
    throw e;
    return new Int(100);
}

function async middleAwait(): Promise<Int> {
    print("Middle await calling inner");
    Promise<Int> p = innerAwait();
    Int result = await p;
    print("Middle got: " + result.toString());
    return new Int(result.getValue() + 50);
}

function async outerAwait(): Promise<String> {
    try {
        print("Outer await calling middle");
        Promise<Int> p = middleAwait();
        Int value = await p;
        return new String("outer success: " + value.toString());
    } catch (Exception e) {
        print("Outer caught: " + e.getMessage());
        return new String("outer handled");
    }
}

function async main(): Promise<void> {
    print("=== Test 1: Simple await exception ===");
    Promise<String> p1 = testSimpleAwait();
    String result1 = await p1;
    print("Result: " + result1.toString());

    print("");
    print("=== Test 2: Multiple awaits ===");
    Promise<String> p2 = testMultipleAwaits();
    String result2 = await p2;
    print("Result: " + result2.toString());

    print("");
    print("=== Test 3: Await in conditional (fail) ===");
    Promise<String> p3 = awaitInConditional(true);
    String result3 = await p3;
    print("Result: " + result3.toString());

    print("");
    print("=== Test 4: Await in conditional (success) ===");
    Promise<String> p4 = awaitInConditional(false);
    String result4 = await p4;
    print("Result: " + result4.toString());

    print("");
    print("=== Test 5: Await in loop ===");
    Promise<String> p5 = awaitInLoop();
    String result5 = await p5;
    print("Result: " + result5.toString());

    print("");
    print("=== Test 6: Sequential awaits ===");
    Promise<String> p6 = sequentialAwaits();
    String result6 = await p6;
    print("Result: " + result6.toString());

    print("");
    print("=== Test 7: Nested awaits ===");
    Promise<String> p7 = outerAwait();
    String result7 = await p7;
    print("Result: " + result7.toString());

    print("");
    print("Await exception test completed");
}

main();
