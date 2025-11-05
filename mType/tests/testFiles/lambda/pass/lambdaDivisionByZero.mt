// Error handling for division by zero test
import * from "../../lib/exceptions/Exception.mt";
interface Function {
    function apply(int x) : int;
}

print("=== Division By Zero Test ===");

// Lambda with safe division
Function safeDivide = x -> {
    if (x == 0) {
        print("Warning: Division by zero, returning 0");
        return 0;
    }
    return 100 / x;
};

print("100 / 10 = " + safeDivide.apply(10));
print("100 / 5 = " + safeDivide.apply(5));
print("100 / 0 = " + safeDivide.apply(0));
print("100 / 2 = " + safeDivide.apply(2));

// Lambda with exception handling for division
Function divideWithException = x -> {
    try {
        if (x == 0) {
            throw new Exception("Division by zero error");
        }
        return 1000 / x;
    } catch (Exception e) {
        print("Caught: " + e.getMessage());
        return -1;
    }
};

print("1000 / 20 = " + divideWithException.apply(20));
print("1000 / 0 = " + divideWithException.apply(0));
print("1000 / 25 = " + divideWithException.apply(25));

// Lambda with modulo by zero check
Function safeModulo = x -> {
    if (x == 0) {
        return -1;
    }
    return 100 % x;
};

print("100 % 7 = " + safeModulo.apply(7));
print("100 % 0 = " + safeModulo.apply(0));
print("100 % 3 = " + safeModulo.apply(3));

// Complex division logic
Function complexDivision = x -> {
    if (x == 0) {
        return 0;
    }
    int result = 500 / x;
    if (result > 100) {
        return 100;
    }
    return result;
};

print("Complex(5): " + complexDivision.apply(5));
print("Complex(2): " + complexDivision.apply(2));
print("Complex(0): " + complexDivision.apply(0));
print("Complex(10): " + complexDivision.apply(10));

print("Division by zero complete");
