import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Diamond operator type inference test
class Container<T> {
    T value;

    public function setValue(T val): void {
        value = val;
    }

    public function getValue(): T {
        return value;
    }
}

function main(): void {
    // Diamond operator inference from constructor context
    Container<Int> intContainer = new Container<>();
    intContainer.setValue(new Int(100));
    print("Int value: " + intContainer.getValue());

    Container<String> strContainer = new Container<>();
    strContainer.setValue(new String("Diamond"));
    print("String value: " + strContainer.getValue());
}

main();
