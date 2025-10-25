// Test throw inside lambda body
import * from "../../lib/exceptions/Exception.mt";
interface Processor {
    function process(int value): string;
}

function main(): void {
    print("Testing lambda with throw");

    Processor riskyProcessor = x -> {
        if (x < 0) {
            throw new Exception("Negative input");
        }
        if (x > 100) {
            throw new Exception("Value too large");
        }
        return "Valid: " + x;
    };

    // Test with valid value
    try {
        string result = riskyProcessor.process(50);
        print(result);
    } catch (Exception e) {
        print("Caught: " + e);
    }

    // Test with negative value
    try {
        string result = riskyProcessor.process(-10);
        print(result);
    } catch (Exception e) {
        print("Caught exception for negative");
    }

    // Test with large value
    try {
        string result = riskyProcessor.process(200);
        print(result);
    } catch (Exception e) {
        print("Caught exception for large");
    }

    print("Lambda throw test completed");
}

main();
