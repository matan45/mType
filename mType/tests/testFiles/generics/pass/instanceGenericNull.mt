import * from "../../lib/primitives/String.mt";

// Test: Generic instance methods with null values
class Container {
    public function <T> store(T value): void {
        if (value == null) {
            print("Stored null value");
        } else {
            print("Stored: " + value);
        }
    }

    public function <T> getDefault(T defaultValue): T {
        return defaultValue;
    }
}

function main(): void {
    Container container = new Container();

    // Test with null
    container.store<String>(null);

    // Test with non-null
    container.store<String>(new String("hello"));

    // Test returning value or null
    String result = container.getDefault<String>(new String("default"));
    print("Result: " + result);

    print("Null handling test passed");
}

main();
