// Generic class with null handling
class Optional<T> {
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

    function clear(): void {
        value = null;
    }
}

function main(): void {
    Optional<string> opt = new Optional<string>();

    print("Initially has value: " + opt.hasValue());

    opt.setValue("Hello");
    print("After setting: " + opt.hasValue());
    print("Value: " + opt.getValue());

    opt.clear();
    print("After clear: " + opt.hasValue());
}

main();