import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Use-site variance annotations
class Box<T> {
    T value;

    public function setValue(T v): void {
        value = v;
    }

    public function getValue(): T {
        return value;
    }
}

class Base {
    String id;

    public constructor(String i) {
        id = i;
    }

    public function getId(): String {
        return id;
    }
}

class Derived extends Base {
    public constructor(String i) : super(i) {
    }
}

function readBox(Box<Base> box): void {
    Base b = box.getValue();
    print("Read: " + b.getId());
}

function main(): void {
    Box<Derived> derivedBox = new Box<Derived>();
    derivedBox.setValue(new Derived(new String("D1")));

    // Use-site variance for safe reading
    readBox(derivedBox);
}

main();
