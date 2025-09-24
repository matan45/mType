import "../../lib/primitives/Int.mt";
import "../../lib/primitives/Float.mt";
import "../../lib/primitives/String.mt";
import "../../lib/primitives/Bool.mt";

// Generic class with various primitive types
class Box<T> {
    T item;

    function set(T value): void {
        item = value;
    }

    function get(): T {
        return item;
    }
}

function main(): void {
    // Test with Int
    Box<Int> intBox = new Box<Int>();
    intBox.set(new Int(123));
    print("Int: " + intBox.get());

    // Test with Float
    Box<Float> floatBox = new Box<Float>();
    floatBox.set(new Float(3.14));
    print("Float: " + floatBox.get());

    // Test with String
    Box<String> stringBox = new Box<String>();
    stringBox.set(new String("Test"));
    print("String: " + stringBox.get());

    // Test with Bool
    Box<Bool> boolBox = new Box<Bool>();
    boolBox.set(new Bool(true));
    print("Bool: " + boolBox.get());
}

main();