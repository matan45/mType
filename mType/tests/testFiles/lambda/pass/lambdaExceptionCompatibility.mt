// Throwing exception subtypes test
import * from "../../lib/exceptions/RuntimeException.mt";

interface Function {
    function apply(int x) : int;
}

print("=== Exception Compatibility Test ===");

Function safeDivider = x -> {
    try {
        if (x == 0) {
            throw new RuntimeException("Division by zero");
        }
        return 100 / x;
    } catch (RuntimeException e) {
        print("Caught exception: " + e.getMessage());
        return -1;
    }
};

print("100 / 10 = " + safeDivider.apply(10));
print("100 / 0 = " + safeDivider.apply(0));
print("100 / 5 = " + safeDivider.apply(5));

// Lambda with nested exception handling
Function complexHandler = x -> {
    try {
        if (x < 0) {
            throw new RuntimeException("Negative input");
        }
        try {
            if (x == 0) {
                throw new RuntimeException("Zero input");
            }
            return 1000 / x;
        } catch (RuntimeException inner) {
            print("Inner catch: " + inner.getMessage());
            return -2;
        }
    } catch (RuntimeException outer) {
        print("Outer catch: " + outer.getMessage());
        return -1;
    }
};

print("complex(10) = " + complexHandler.apply(10));
print("complex(0) = " + complexHandler.apply(0));
print("complex(-5) = " + complexHandler.apply(-5));

print("Exception compatibility complete");
