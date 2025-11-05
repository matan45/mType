// Passing lambda to generic method test
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
interface Function<T, R> {
    function apply(T input) : R;
}

class Processor {
    public function <T, R> process(T[] items, Function<T, R> mapper) : R[] {
        R[] result = new R[items.length];
        for (int i = 0; i < items.length; i = i + 1) {
            result[i] = mapper.apply(items[i]);
        }
        return result;
    }
}

print("=== Generic Method Inference Test ===");

Processor p = new Processor();
Int[] numbers = [new Int(1), new Int(2), new Int(3), new Int(4), new Int(5)];

// Lambda inferred as Function<int, String>
String[] strings = p.process<Int,String>(numbers, x -> new String("Value:" + x.value));
for (int i = 0; i < strings.length; i = i + 1) {
    print(strings[i].toString());
}

// Lambda inferred as Function<int, int>
Int[] doubled = p.process<Int,Int>(numbers, x -> new Int(x.value * 2));
for (int i = 0; i < doubled.length; i = i + 1) {
    print("Doubled: " + doubled[i].toString());
}

print("Generic method inference complete");
