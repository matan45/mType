// Test: Lambda execution within try-catch blocks
// Expected: Should compile and run successfully
import * from "../../lib/exceptions/Exception.mt";

interface Calculator {
    public function calculate(int a, int b): int;
}

interface Processor {
    public function process(string input): string;
}

function main(): void {
    print("Testing lambda execution in try-catch blocks");

    // Lambda with division that may throw
    Calculator divider = (a, b) -> {
        if (b == 0) {
            throw new Exception("Division by zero");
        }
        return a / b;
    };

    // Execute lambda inside try-catch
    try {
        int result = divider.calculate(10, 2);
        print("Division result: " + result);
    } catch (Exception e) {
        print("Division error: " + e.getMessage());
    }

    try {
        int result = divider.calculate(10, 0);
        print("Division result: " + result);
    } catch (Exception e) {
        print("Division error: " + e.getMessage());
    }

    // Lambda that internally handles exceptions
    Processor safeProcessor = input -> {
        try {
            if (input.length() == 0) {
                throw new Exception("Empty input");
            }
            return "Processed: " + input.toUpperCase();
        } catch (Exception e) {
            return "Error: " + e.getMessage();
        }
    };

    // Execute safe lambda (no outer try-catch needed)
    string result1 = safeProcessor.process("hello");
    print(result1);

    string result2 = safeProcessor.process("");
    print(result2);

    // Nested try-catch with lambda
    try {
        Calculator multiplier = (a, b) -> {
            if (a > 1000 || b > 1000) {
                throw new Exception("Values too large");
            }
            return a * b;
        };

        try {
            int result = multiplier.calculate(5, 10);
            print("Multiplication result: " + result);
        } catch (Exception e) {
            print("Inner catch: " + e.getMessage());
        }

        try {
            int result = multiplier.calculate(2000, 3);
            print("Multiplication result: " + result);
        } catch (Exception e) {
            print("Inner catch: " + e.getMessage());
        }

        print("Nested try-catch completed");
    } catch (Exception e) {
        print("Outer catch: " + e.getMessage());
    }

    // Lambda defined in try block, used in catch
    Exception capturedException = null;

    try {
        throw new Exception("Test exception for lambda");
    } catch (Exception e) {
        capturedException = e;
        Processor errorFormatter = msg -> {
            return "Error [" + capturedException.getMessage() + "]: " + msg;
        };

        string formatted = errorFormatter.process("Additional context");
        print(formatted);
    }

    print("Lambda in try-catch test completed");
}

main();
