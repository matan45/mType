// Test that string primitive type is rejected in generics
// This should fail with "Invalid type arguments" error

class Container<T> {
    T value;

    function setValue(T val): void {
        value = val;
    }

    function getValue(): T {
        return value;
    }
}

function main(): void {
    // This should fail - string is a primitive type
    Container<string> stringContainer = new Container<string>();
    stringContainer.setValue("test");
    print(stringContainer.getValue());
}

main();