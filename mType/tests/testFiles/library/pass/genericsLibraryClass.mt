import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

class Box<T> {
    T value;

    public constructor(T value) {
        this.value = value;
    }

    public function getValue(): T {
        return this.value;
    }
}

Box<Int> intBox = new Box<Int>(new Int(42));
print(intBox.getValue());

Box<String> strBox = new Box<String>("hello");
print(strBox.getValue());
