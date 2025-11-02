// Test: Finally block in async functions
// Expected: Finally blocks execute in all scenarios (success, exception, return)
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/exceptions/RuntimeException.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";

// Test 1: Finally with successful execution
function async finallySuccess(): Promise<String> {
    try {
        print("Try block executing");
        return new String("success");
    } finally {
        print("Finally block executed (success)");
    }
}

// Test 2: Finally with exception
function async finallyWithException(): Promise<String> {
    try {
        print("About to throw");
        Exception e = new Exception("Test error");
        throw e;
        return new String("unreachable");
    } catch (Exception e) {
        print("Caught: " + e.getMessage());
        return new String("caught");
    } finally {
        print("Finally block executed (with exception)");
    }
}

// Test 3: Finally without catch
function async finallyNoCatch(): Promise<String> {
    try {
        print("Try without catch");
        return new String("completed");
    } finally {
        print("Finally without catch executed");
    }
}

// Test 4: Multiple finally blocks (nested)
function async nestedFinally(): Promise<String> {
    try {
        print("Outer try");
        try {
            print("Inner try");
            return new String("inner return");
        } finally {
            print("Inner finally");
        }
    } finally {
        print("Outer finally");
    }
}

// Test 5: Finally with exception propagation
function async finallyPropagateException(): Promise<String> {
    try {
        print("Throwing exception");
        RuntimeException e = new RuntimeException("Will propagate");
        throw e;
        return new String("never");
    } finally {
        print("Finally before propagation");
    }
}

function async catchPropagated(): Promise<String> {
    try {
        Promise<String> p = finallyPropagateException();
        String result = await p;
        return result;
    } catch (RuntimeException e) {
        print("Caught propagated: " + e.getMessage());
        return new String("handled");
    }
}

// Test 6: Finally with multiple returns
function async multipleReturnsFinally(Int choice): Promise<String> {
    try {
        print("Choice: " + choice.toString());
        if (choice.toInt() == 1) {
            return new String("first");
        }
        if (choice.toInt() == 2) {
            Exception e = new Exception("Error case");
            throw e;
        }
        return new String("default");
    } catch (Exception e) {
        print("Caught in multiple returns: " + e.getMessage());
        return new String("error");
    } finally {
        print("Finally in multiple returns");
    }
}

// Test 7: Finally with early return
function async earlyReturnFinally(bool shouldReturn): Promise<Int> {
    try {
        print("Early return test");
        if (shouldReturn) {
            print("Returning early");
            return new Int(100);
        }
        print("Normal flow");
        return new Int(200);
    } finally {
        print("Finally after early return check");
    }
}

// Test 8: Nested try-catch-finally
function async complexNesting(): Promise<String> {
    try {
        print("Outer try start");
        try {
            print("Middle try start");
            try {
                print("Inner try start");
                Exception e = new Exception("Deep exception");
                throw e;
            } finally {
                print("Inner finally");
            }
        } catch (Exception e) {
            print("Middle catch: " + e.getMessage());
            return new String("middle");
        } finally {
            print("Middle finally");
        }
    } finally {
        print("Outer finally");
    }
}

function async main(): Promise<void> {
    print("=== Test 1: Finally with success ===");
    Promise<String> p1 = finallySuccess();
    String result1 = await p1;
    print("Result: " + result1.toString());

    print("");
    print("=== Test 2: Finally with exception ===");
    Promise<String> p2 = finallyWithException();
    String result2 = await p2;
    print("Result: " + result2.toString());

    print("");
    print("=== Test 3: Finally without catch ===");
    Promise<String> p3 = finallyNoCatch();
    String result3 = await p3;
    print("Result: " + result3.toString());

    print("");
    print("=== Test 4: Nested finally ===");
    Promise<String> p4 = nestedFinally();
    String result4 = await p4;
    print("Result: " + result4.toString());

    print("");
    print("=== Test 5: Finally with propagation ===");
    Promise<String> p5 = catchPropagated();
    String result5 = await p5;
    print("Result: " + result5.toString());

    print("");
    print("=== Test 6: Multiple returns - first ===");
    Promise<String> p6 = multipleReturnsFinally(new Int(1));
    String result6 = await p6;
    print("Result: " + result6.toString());

    print("");
    print("=== Test 7: Multiple returns - error ===");
    Promise<String> p7 = multipleReturnsFinally(new Int(2));
    String result7 = await p7;
    print("Result: " + result7.toString());

    print("");
    print("=== Test 8: Early return - true ===");
    Promise<Int> p8 = earlyReturnFinally(true);
    Int result8 = await p8;
    print("Result: " + result8.toString());

    print("");
    print("=== Test 9: Early return - false ===");
    Promise<Int> p9 = earlyReturnFinally(false);
    Int result9 = await p9;
    print("Result: " + result9.toString());

    print("");
    print("=== Test 10: Complex nesting ===");
    Promise<String> p10 = complexNesting();
    String result10 = await p10;
    print("Result: " + result10.toString());

    print("");
    print("Finally block test completed");
    return null;
}

main();
