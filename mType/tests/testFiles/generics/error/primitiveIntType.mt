// Test that int primitive type is rejected in generics
// This should fail with "Invalid type arguments" error

class NumberContainer<T> {
    T data;

    constructor(T value) {
        data = value;
    }

    function getData(): T {
        return data;
    }
}

function main(): void {
    // This should fail - int is a primitive type
    NumberContainer<int> intContainer = new NumberContainer<int>(42);
    print(intContainer.getData());
}

main();