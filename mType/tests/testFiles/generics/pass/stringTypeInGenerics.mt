// Test that String type is now allowed in generics (Pure OOP)
// This should PASS - String is now a proper object type

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
    // String is now an object type and can be used in generics!
    Container<String> stringContainer = new Container<String>();
    stringContainer.setValue(new String("test"));
    String result = stringContainer.getValue();
    print(result.toString());
}

main();
