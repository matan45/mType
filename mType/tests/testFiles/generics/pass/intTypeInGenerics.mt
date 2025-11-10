// Test that Int type is now allowed in generics (Pure OOP)
// This should PASS - Int is now a proper object type

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
    // Int is now an object type and can be used in generics!
    NumberContainer<Int> intContainer = new NumberContainer<Int>(new Int(42));
    Int result = intContainer.getData();
    print(result.toString());
}

main();
