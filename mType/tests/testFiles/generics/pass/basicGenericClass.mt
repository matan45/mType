// Basic generic class definition and instantiation
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
    Container<int> intContainer = new Container<int>();
    intContainer.setValue(42);
    int result = intContainer.getValue();
    print("Result: " + result);
}

main();