// Generic class with constructors
class Container<T> {
    T value;

    function setValue(T newValue): void {
        value = newValue;
    }

    function getValue(): T {
        return value;
    }
}

function main(): void {
    // Test default constructor
    Container<int> container1 = new Container<int>();
    container1.setValue(42);
    print("Container1: " + container1.getValue());

    // Test with string
    Container<string> container2 = new Container<string>();
    container2.setValue("Hello World");
    print("Container2: " + container2.getValue());
}

main();