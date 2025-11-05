// Return in finally block of lambda test
import * from "../../lib/exceptions/RuntimeException.mt";
interface Function {
    function apply(int x) : int;
}

print("=== Finally With Return Test ===");

// Return in finally overrides try return
Function finallyReturner = x -> {
    try {
        if (x < 0) {
            throw new RuntimeException("Negative");
        }
        return x * 2;
    } catch (RuntimeException e) {
        print("Exception: " + e.getMessage());
        return -1;
    } finally {
        print("Finally block executed for " + x);
    }
};

print("Result(5): " + finallyReturner.apply(5));
print("Result(-3): " + finallyReturner.apply(-3));

// Finally without return
Function normalFinally = x -> {
    int result = 0;
    try {
        result = x * x;
    } catch (RuntimeException e) {
        result = -1;
    } finally {
        print("Cleanup for " + x);
    }
    return result;
};

print("Square(4): " + normalFinally.apply(4));
print("Square(7): " + normalFinally.apply(7));

// Multiple finally blocks
Function nestedFinally = x -> {
    try {
        try {
            return x + 10;
        } finally {
            print("Inner finally");
        }
    } finally {
        print("Outer finally");
    }
};

print("Nested(5): " + nestedFinally.apply(5));

print("Finally with return complete");
