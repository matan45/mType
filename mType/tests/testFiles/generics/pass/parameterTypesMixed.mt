// Test mixed parameter types - all together
import * from "../../lib/primitives/String.mt";
class Container<T> {
    T value;

    constructor(T val) {
        value = val;
    }

    public function getValue(): T {
        return value;
    }
}

// Function with mixed parameter types
function complexFunction(
    int count,
    string message,
    bool flag,
    int[] numbers,
    Container<String> container
): void {
    print(count);
    print(message);
    print(flag);
    print(numbers[0]);
    print(container.getValue().toString());
}

// Call with all types
int[] arr = [100, 200, 300];
Container<String> strContainer = new Container<String>(new String("container data"));

complexFunction(42, "test", true, arr, strContainer);
