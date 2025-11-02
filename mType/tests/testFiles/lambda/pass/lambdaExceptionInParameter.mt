// Exception during parameter evaluation test
interface Function {
    function apply(int x) : int;
}

class Calculator {
    function riskyOperation(int x) : int {
        if (x < 0) {
            throw "Negative input not allowed";
        }
        return x * x;
    }
}

print("=== Exception In Parameter Test ===");

Calculator calc = new Calculator();

Function processor = x -> {
    try {
        int value = calc.riskyOperation(x);
        return value + 10;
    } catch (String e) {
        print("Caught in lambda: " + e);
        return -1;
    }
};

print("Process(5): " + processor.apply(5));
print("Process(-3): " + processor.apply(-3));
print("Process(10): " + processor.apply(10));

// Exception in lambda invocation argument
Function outer = x -> {
    Function inner = y -> y * 2;
    try {
        int result = inner.apply(calc.riskyOperation(x));
        return result;
    } catch (String e) {
        print("Error during invocation: " + e);
        return 0;
    }
};

print("Outer(4): " + outer.apply(4));
print("Outer(-2): " + outer.apply(-2));

print("Exception in parameter complete");
