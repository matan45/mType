// Error: Invalid return type assignment
class Container<T> {
    T value;

    function getValue(): T {
        return value;
    }
}

function main(): void {
    Container<int> container = new Container<int>();
    // Should fail: trying to assign int return to string variable
    string result = container.getValue();
}

main();