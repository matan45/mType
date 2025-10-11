import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Generic class with constructors
class Container<T> {
    T value;

    public function setValue(T newValue): void {
        value = newValue;
    }

    public function getValue(): T {
        return value;
    }
}

function main(): void {
    // Test default constructor
    Container<Int> container1 = new Container<Int>();
    container1.setValue(new Int(42));
    print("Container1: " + container1.getValue());

    // Test with String
    Container<String> container2 = new Container<String>();
    container2.setValue(new String("Hello World"));
    print("Container2: " + container2.getValue());
}

main();