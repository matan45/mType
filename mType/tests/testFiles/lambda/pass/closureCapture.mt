// Closure variable capture test
interface Function {
    function apply(x: int) : int;
}

print("=== Closure Variable Capture Test ===");

function testClosure() : int {
    int localVar = 10;
    int multiplier = 5;

    Function adder = x -> x + localVar;
    Function calculator = x -> x * multiplier + localVar;

    print("Captured localVar (10): " + adder.apply(5));
    print("Complex capture: " + calculator.apply(3));

    return adder.apply(0); // Should return 10 (just localVar)
}

int result = testClosure();
print("Final result: " + result);