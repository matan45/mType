import "../../lib/primitives/Int.mt";

// Basic generic class definition and instantiation
class Box<T> {
    T value;

    public function setValue(T newValue): void {
        value = newValue;
    }

    public function getValue(): T {
        return value;
    }
}

function main(): void {
    Box<Int> intBox = new Box<Int>();
    intBox.setValue(new Int(42));
    Int result = intBox.getValue();
    print("Result: " + result);
}

main();