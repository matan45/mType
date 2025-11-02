import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Lambda capturing generic variables
class Accumulator<T> {
    T sum;
    function(T, T): T combiner;

    public function Accumulator(T initial, function(T, T): T comb) {
        sum = initial;
        combiner = comb;
    }

    public function add(T value): void {
        sum = combiner(sum, value);
    }

    public function getSum(): T {
        return sum;
    }
}

function main(): void {
    Accumulator<Int> intAccum = new Accumulator<Int>(
        new Int(0),
        function(Int a, Int b): Int {
            return new Int(a.value + b.value);
        }
    );

    intAccum.add(new Int(10));
    intAccum.add(new Int(20));
    intAccum.add(new Int(30));

    print("Sum: " + intAccum.getSum());

    Accumulator<String> strAccum = new Accumulator<String>(
        new String(""),
        function(String a, String b): String {
            return new String(a + b);
        }
    );

    strAccum.add(new String("Hello"));
    strAccum.add(new String(" "));
    strAccum.add(new String("World"));

    print("Concat: " + strAccum.getSum());
}

main();
