// Pass test: Final generic class can be defined and used
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

final class FinalBox<T> {
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
}

// Can create instances of final generic class
FinalBox<Int> intBox = new FinalBox<Int>(new Int(42));
print(intBox.getValue());

FinalBox<String> strBox = new FinalBox<String>(new String("Hello"));
print(strBox.getValue());

strBox.setValue(new String("World"));
print(strBox.getValue());
