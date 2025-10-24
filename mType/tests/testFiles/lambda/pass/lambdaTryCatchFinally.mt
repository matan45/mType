// Test exception handling inside lambda

interface Processor {
    function process(int value): string;
}

function main(): void {
    print("Testing lambda with try-catch-finally");

    // Lambda with try-catch
    Processor processor1 = (int x) -> {
        try {
            if (x < 0) {
                throw new Exception("Negative value");
            }
            return "Success: " + x;
        } catch (Exception e) {
            return "Error caught: " + x;
        }
    };

    print(processor1.process(10));
    print(processor1.process(-5));

    // Lambda with try-finally
    Processor processor2 = (int x) -> {
        string result = "";

        try {
            result = "Processing: " + x;
            if (x == 0) {
                return "Zero value";
            }
        } finally {
            print("Finally executed for: " + x);
        }

        return result;
    };

    print(processor2.process(5));
    print(processor2.process(0));

    print("Lambda exception handling test completed");
}

main();
