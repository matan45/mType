import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Lambda capturing generic variables
interface BinaryFunc<T> {
    function apply(T a, T b): T;
}

class Accumulator<T> {
    T sum;
    BinaryFunc<T> combiner;

    public constructor(T initial, BinaryFunc<T> comb) {
        sum = initial;
        combiner = comb;
    }

    public function add(T value): void {
        sum = combiner.apply(sum, value);
    }

    public function getSum(): T {
        return sum;
    }
}

function main(): void {
    Accumulator<Int> intAccum = new Accumulator<Int>(
        new Int(0),
        (a, b) -> new Int(a.getValue() + b.getValue())
    );

    intAccum.add(new Int(10));
    intAccum.add(new Int(20));
    intAccum.add(new Int(30));

    print("Sum: " + intAccum.getSum());

    Accumulator<String> strAccum = new Accumulator<String>(
        new String(""),
        (a, b) -> new String(a + b)
    );

    strAccum.add(new String("Hello"));
    strAccum.add(new String(" "));
    strAccum.add(new String("World"));

    print("Concat: " + strAccum.getSum());
}

main();
