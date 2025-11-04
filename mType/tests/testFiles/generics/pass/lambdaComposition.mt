import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Lambda composition with generics
interface Transformer<T> {
    function transform(T input): T;
}

class Pipeline<T> {
    T value;

    public constructor(T initial) {
        value = initial;
    }

    public function map(Transformer<T> transformer): Pipeline<T> {
        value = transformer.transform(value);
        return this;
    }

    public function getValue(): T {
        return value;
    }
}

function main(): void {
    Pipeline<Int> intPipeline = new Pipeline<Int>(new Int(10));

    Int result = intPipeline
        .map(x -> new Int(x.value * 2))
        .map(x -> new Int(x.value + 5))
        .map(x -> new Int(x.value * 3))
        .getValue();

    print("Result: " + result);
}

main();
