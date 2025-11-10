// Test generic array variance
print("Testing generic array variance");

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
}

// Create array of generic boxes with Int wrapper
Box<Int>[] intBoxes = new Box<Int>[3];
intBoxes[0] = new Box<Int>(new Int(10));
intBoxes[1] = new Box<Int>(new Int(20));
intBoxes[2] = new Box<Int>(new Int(30));

print("Box<Int> array:");
for (int i = 0; i < intBoxes.length; i++) {
    print("  intBoxes[" + i + "].value = " + intBoxes[i].getValue().getValue());
}

// String wrapper class for generics
Box<String>[] stringBoxes = new Box<String>[2];
stringBoxes[0] = new Box<String>(new String("Hello"));
stringBoxes[1] = new Box<String>(new String("World"));

print("Box<String> array:");
for (int i = 0; i < stringBoxes.length; i++) {
    print("  stringBoxes[" + i + "].value = " + stringBoxes[i].getValue().getValue());
}

print("Generic array variance test completed");
