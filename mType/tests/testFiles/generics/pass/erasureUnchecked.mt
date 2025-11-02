import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Unchecked type conversions
class Box<T> {
    T value;

    public function setValue(T v): void {
        value = v;
    }

    public function getValue(): T {
        return value;
    }
}

class Container {
    Box<Object> box;

    public function setBox(Box<Object> b): void {
        box = b;
    }

    public function getBox(): Box<Object> {
        return box;
    }
}

function main(): void {
    Box<String> stringBox = new Box<String>();
    stringBox.setValue(new String("test"));

    // Unchecked conversion for generic assignment
    Container container = new Container();
    container.setBox(stringBox);

    Box<Object> retrieved = container.getBox();
    print("Retrieved box");
}

main();
