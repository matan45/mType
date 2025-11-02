import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Generic method with raw types
class Container<T> {
    T value;

    public function setValue(T v): void {
        value = v;
    }

    public function getValue(): T {
        return value;
    }

    public function <R> transform(function(T): R fn): R {
        return fn(value);
    }
}

function main(): void {
    Container<Int> intContainer = new Container<Int>();
    intContainer.setValue(new Int(42));

    String result = intContainer.transform(function(Int x): String {
        return new String("Transformed: " + x);
    });

    print(result);
}

main();
