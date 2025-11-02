import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Lambda composition with generics
class Pipeline<T> {
    T value;

    public function Pipeline(T initial) {
        value = initial;
    }

    public function map(function(T): T transformer): Pipeline<T> {
        value = transformer(value);
        return this;
    }

    public function getValue(): T {
        return value;
    }
}

function main(): void {
    Pipeline<Int> intPipeline = new Pipeline<Int>(new Int(10));

    Int result = intPipeline
        .map(function(Int x): Int { return new Int(x.value * 2); })
        .map(function(Int x): Int { return new Int(x.value + 5); })
        .map(function(Int x): Int { return new Int(x.value * 3); })
        .getValue();

    print("Result: " + result);
}

main();
