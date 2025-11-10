import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Runtime type checking with type erasure
class Box<T> {
    T value;
    String typeName;

    public function setValue(T v, String name): void {
        value = v;
        typeName = name;
    }

    public function getValue(): T {
        return value;
    }

    public function getTypeName(): String {
        return typeName;
    }
}

function main(): void {
    Box<Int> intBox = new Box<Int>();
    intBox.setValue(new Int(42), new String("Int"));

    Box<String> strBox = new Box<String>();
    strBox.setValue(new String("hello"), new String("String"));

    // Runtime type information preserved via typeName
    print("Type: " + intBox.getTypeName() + ", Value: " + intBox.getValue());
    print("Type: " + strBox.getTypeName() + ", Value: " + strBox.getValue());
}

main();
