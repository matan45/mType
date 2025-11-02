// Test: Multiple lambdas with exception handling in a chain
// Expected: Should compile and run successfully
import * from "../../lib/exceptions/Exception.mt";

interface IntProcessor {
    function process(int value): int;
}

interface StringProcessor {
    function process(string value): string;
}

function main(): void {
    print("Testing lambda chain with exception handling");

    // First lambda in chain
    IntProcessor doubler = x -> {
        if (x < 0) {
            throw new Exception("Doubler: negative input");
        }
        return x * 2;
    };

    // Second lambda in chain
    IntProcessor incrementer = x -> {
        if (x > 100) {
            throw new Exception("Incrementer: value too large");
        }
        return x + 1;
    };

    // Third lambda in chain
    IntProcessor validator = x -> {
        if (x % 2 != 0) {
            throw new Exception("Validator: odd number detected");
        }
        return x;
    };

    // Test successful chain
    try {
        int result = doubler.process(10);
        print("After doubler: " + result);
        result = incrementer.process(result);
        print("After incrementer: " + result);
        result = validator.process(result);
        print("After validator: " + result);
        print("Chain completed successfully");
    } catch (Exception e) {
        print("Chain failed: " + e.getMessage());
    }

    // Test chain with exception in first lambda
    try {
        int result = doubler.process(-5);
        result = incrementer.process(result);
        result = validator.process(result);
        print("Chain completed");
    } catch (Exception e) {
        print("Chain failed at doubler: " + e.getMessage());
    }

    // Test chain with exception in second lambda
    try {
        int result = doubler.process(60);
        print("After doubler: " + result);
        result = incrementer.process(result);
        print("After incrementer: " + result);
    } catch (Exception e) {
        print("Chain failed at incrementer: " + e.getMessage());
    }

    // Test chain with exception in third lambda
    try {
        int result = doubler.process(5);
        print("After doubler: " + result);
        result = incrementer.process(result);
        print("After incrementer: " + result);
        result = validator.process(result);
        print("After validator: " + result);
    } catch (Exception e) {
        print("Chain failed at validator: " + e.getMessage());
    }

    print("Lambda chain exception test completed");
}

main();
