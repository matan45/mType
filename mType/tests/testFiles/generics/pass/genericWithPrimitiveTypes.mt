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
    // Test with int
    Box<int> intBox = new Box<int>();
    intBox.set(123);
    print("Int: " + intBox.get());

    // Test with float
    Box<float> floatBox = new Box<float>();
    floatBox.set(3.14);
    print("Float: " + floatBox.get());

    // Test with string
    Box<string> stringBox = new Box<string>();
    stringBox.set("Test");
    print("String: " + stringBox.get());

    // Test with bool
    Box<bool> boolBox = new Box<bool>();
    boolBox.set(true);
    print("Bool: " + boolBox.get());
}

main();