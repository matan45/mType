import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
// Test generic return types
class Box<T> {
    T value;

    constructor(T val) {
        value = val;
    }

    public function getValue(): T {
        return value;
    }
}

function <T> createBox(T item): Box<T> {
    return new Box<T>(item);
}

function <T> getBoxValue(Box<T> box): T {
    return box.getValue();
}

// Test functions
Box<Int> intBox = createBox<Int>(new Int(100));
print(getBoxValue<Int>(intBox));

Box<String> strBox = createBox<String>(new String("boxed"));
print(getBoxValue<String>(strBox));
