// Error: Type mismatch in generic method call
class Container<T> {
    T value;

    function setValue(T newValue): void {
        value = newValue;
    }
}

function main(): void {
    Container<int> container = new Container<int>();
    // Should fail: trying to pass string to method expecting int
    container.setValue("wrong type");
}

main();