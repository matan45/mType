// Test basic generic class serialization/deserialization
class Container<T> {
    T value;

    function setValue(T newValue): void {
        value = newValue;
    }

    function getValue(): T {
        return value;
    }

    function hasValue(): bool {
        return value != null;
    }
}

function main(): void {
    Container<int> intContainer = new Container<int>();
    intContainer.setValue(42);
    print("Int container: " + intContainer.getValue());

    Container<string> stringContainer = new Container<string>();
    stringContainer.setValue("Hello World");
    print("String container: " + stringContainer.getValue());

    Container<bool> boolContainer = new Container<bool>();
    boolContainer.setValue(true);
    print("Bool container: " + boolContainer.getValue());
}

main();