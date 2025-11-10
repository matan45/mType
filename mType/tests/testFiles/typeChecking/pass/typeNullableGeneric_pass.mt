// Test nullable generic type parameters
// Validates that generic types can be instantiated with nullable type arguments
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

class Box<T> {
    T value;

    constructor(T v) {
        value = v;
    }

    public function getValue(): T {
        return value;
    }

    public function setValue(T v): void {
        value = v;
    }

    public function hasValue(): bool {
        return value != null;
    }
}

class Container<T> {
    T item;

    constructor() {
        item = null;
    }

    public function set(T i): void {
        item = i;
    }

    public function get(): T {
        return item;
    }
}

function main(): void {
    print("Testing nullable generic type parameters");

    // Generic with nullable string
    Box<String> strBox = new Box<String>("Hello");
    print("String box value: " + strBox.getValue());
    strBox.setValue(null);
    if (strBox.hasValue()) {
        print("Box has value");
    } else {
        print("Box value is null");
    }

    // Generic with nullable object
    Container<Box<Int>> boxContainer = new Container<Box<Int>>();
    if (boxContainer.get() == null) {
        print("Container is empty");
    }

    boxContainer.set(new Box<Int>(new Int(42)));
    if (boxContainer.get() != null) {
        print("Container has box with value: " + boxContainer.get().getValue().getValue());
    }

    // Null assignment to generic
    boxContainer.set(null);
    if (boxContainer.get() == null) {
        print("Container is empty again");
    }

    print("Nullable generic test completed");
}

main();
