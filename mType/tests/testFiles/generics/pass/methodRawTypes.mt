import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Generic method with raw types
interface Transformer<T, R> {
    function apply(T input): R;
}

class Container<T> {
    T value;

    public function setValue(T v): void {
        value = v;
    }

    public function getValue(): T {
        return value;
    }

    public function <R> transform(Transformer<T, R> fn): R {
        return fn.apply(value);
    }
}

function main(): void {
    Container<Int> intContainer = new Container<Int>();
    intContainer.setValue(new Int(42));

    String result = intContainer.transform<String>(x -> new String("Transformed: " + x));

    print(result.toString());
}

main();
