// Test that Float type is now allowed in generics (Pure OOP)
// This should PASS - Float is now a proper object type

class Box<T> {
    T item;

    constructor(T value) {
        item = value;
    }

    function getItem(): T {
        return item;
    }
}

function main(): void {
    // Float is now an object type and can be used in generics!
    Box<Float> floatBox = new Box<Float>(new Float(3.14));
    Float result = floatBox.getItem();
    print(result.toString());
}

main();
