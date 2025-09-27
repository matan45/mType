// Test that float primitive type is rejected in generics
// This should fail with "Invalid type arguments" error

class FloatHolder<T> {
    T value;

    function store(T val): void {
        value = val;
    }

    function retrieve(): T {
        return value;
    }
}

function main(): void {
    // This should fail - float is a primitive type
    FloatHolder<float> floatHolder = new FloatHolder<float>();
    floatHolder.store(3.14);
    print(floatHolder.retrieve());
}

main();