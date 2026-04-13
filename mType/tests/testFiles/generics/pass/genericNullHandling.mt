import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Bool.mt";

// Generic class with null handling
class Optional<T> {
    T? value;

    public function setValue(T newValue): void {
        value = newValue;
    }

    public function getValue(): T? {
        return value;
    }

    public function hasValue(): Bool {
        return new Bool(value != null);
    }

    public function clear(): void {
        value = null;
    }
}

function main(): void {
    Optional<String> opt = new Optional<String>();

    print("Initially has value: " + opt.hasValue());

    opt.setValue(new String("Hello"));
    print("After setting: " + opt.hasValue());
    print("Value: " + opt.getValue());

    opt.clear();
    print("After clear: " + opt.hasValue());
}

main();