// Test varargs method overload resolution
class Logger {
    // Single parameter method
    public static function log(string msg): void {
        print("Log: " + msg);
    }

    // Two parameter method
    public static function log(string msg, int level): void {
        print("Log [" + level + "]: " + msg);
    }

    // Three parameter method
    public static function log(string msg, int level, bool critical): void {
        string prefix = "Log [" + level + "]";
        if (critical) {
            prefix = prefix + " CRITICAL";
        }
        print(prefix + ": " + msg);
    }
}

// Test with array overloads
class ArrayProcessor {
    // Single element
    public function process(int element): string {
        return "Single: " + element;
    }

    // Array of elements
    public function process(int[] elements): string {
        return "Array: " + elements.length + " elements";
    }
}

function main(): void {
    print("Testing varargs method overload resolution");

    // Test Logger with different parameter counts
    Logger.log("Simple message");
    Logger.log("Info message", 1);
    Logger.log("Error message", 3, true);
    Logger.log("Warning message", 2, false);

    // Test ArrayProcessor
    ArrayProcessor processor = new ArrayProcessor();
    print(processor.process(42));

    int[] numbers = {10, 20, 30};
    print(processor.process(numbers));

    print("Varargs overload test completed");
}

main();
