// Test: Exception propagation through async chains
// Expected: Exceptions propagate correctly through multiple async layers
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/exceptions/RuntimeException.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";

class DataException extends Exception {
    public Int errorCode;

    public constructor(string msg, Int code) {
        super(msg);
        errorCode = code;
    }

    public function getCode(): Int {
        return errorCode;
    }
}

// Bottom level - throws exception
function async deepFunction(): Promise<Int> {
    print("Deep function executing");
    DataException e = new DataException("Deep error", new Int(500));
    throw e;
    return new Int(1);
}

// Middle level - propagates without handling
function async middleFunction(): Promise<Int> {
    print("Middle function executing");
    Promise<Int> p = deepFunction();
    Int result = await p;
    print("Middle function completed: " + result.toString());
    return result;
}

// Top level - catches exception
function async topFunction(): Promise<String> {
    print("Top function executing");
    try {
        Promise<Int> p = middleFunction();
        Int value = await p;
        return new String("success: " + value.toString());
    } catch (DataException e) {
        print("Caught at top: " + e.getMessage());
        print("Error code: " + e.getCode().toString());
        return new String("error handled");
    }
}

// Test with multiple async calls in sequence
function async step1(): Promise<Int> {
    print("Step 1");
    return new Int(10);
}

function async step2(Int input): Promise<Int> {
    print("Step 2: " + input.toString());
    Exception e = new Exception("Error in step 2");
    throw e;
    return new Int(20);
}

function async step3(Int input): Promise<Int> {
    print("Step 3: " + input.toString());
    return new Int(30);
}

function async chainedExecution(): Promise<String> {
    try {
        print("Starting chain");
        Promise<Int> p1 = step1();
        Int r1 = await p1;

        Promise<Int> p2 = step2(r1);
        Int r2 = await p2;

        Promise<Int> p3 = step3(r2);
        Int r3 = await p3;

        return new String("completed: " + r3.toString());
    } catch (Exception e) {
        print("Chain interrupted: " + e.getMessage());
        return new String("chain failed");
    }
}

// Test exception in middle of nested awaits
function async innerAsync(): Promise<Int> {
    print("Inner async");
    RuntimeException e = new RuntimeException("Inner error");
    throw e;
    return new Int(100);
}

function async outerAsync(): Promise<Int> {
    print("Outer async start");
    Promise<Int> p = innerAsync();
    Int value = await p;
    print("Outer async end: " + value.toString());
    return new Int(value.toInt() + 50);
}

function async wrapperAsync(): Promise<String> {
    try {
        print("Wrapper start");
        Promise<Int> p = outerAsync();
        Int result = await p;
        return new String("wrapper success: " + result.toString());
    } catch (RuntimeException e) {
        print("Wrapper caught: " + e.getMessage());
        return new String("wrapper handled");
    }
}

function async main(): Promise<void> {
    print("=== Test 1: Three-layer propagation ===");
    Promise<String> p1 = topFunction();
    String result1 = await p1;
    print("Result: " + result1.toString());

    print("");
    print("=== Test 2: Chained async execution ===");
    Promise<String> p2 = chainedExecution();
    String result2 = await p2;
    print("Result: " + result2.toString());

    print("");
    print("=== Test 3: Nested await propagation ===");
    Promise<String> p3 = wrapperAsync();
    String result3 = await p3;
    print("Result: " + result3.toString());

    print("");
    print("Nested exception test completed");
    return null;
}

main();
