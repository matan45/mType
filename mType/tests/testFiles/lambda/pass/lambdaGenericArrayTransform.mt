// Transform generic arrays with lambdas
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/Bool.mt";
interface Function<T, R> {
    function apply(T input) : R;
}

class ArrayUtils {
    public function <T, R> map(T[] input, Function<T, R> transformer) : R[] {
        R[] result = new R[input.length];
        for (int i = 0; i < input.length; i = i + 1) {
            result[i] = transformer.apply(input[i]);
        }
        return result;
    }

    public function <T> filter(T[] input, Function<T, Bool> predicate) : T[] {
        int count = 0;
        for (int i = 0; i < input.length; i = i + 1) {
            if (predicate.apply(input[i]).getValue()) {
                count = count + 1;
            }
        }

        T[] result = new T[count];
        int idx = 0;
        for (int i = 0; i < input.length; i = i + 1) {
            if (predicate.apply(input[i]).getValue()) {
                result[idx] = input[i];
                idx = idx + 1;
            }
        }
        return result;
    }
}

print("=== Generic Array Transform Test ===");

ArrayUtils utils = new ArrayUtils();
Int[] numbers = [new Int(1), new Int(2), new Int(3), new Int(4), new Int(5)];

// Map int[] to String[]
String[] strings = utils.map<Int,String>(numbers, x -> new String("Item-" + x.getValue()));
for (int i = 0; i < strings.length; i = i + 1) {
    print(strings[i].toString());
}

// Filter even numbers
Int[] evens = utils.filter<Int>(numbers, x -> new Bool(x.getValue() % 2 == 0));
print("Evens:");
for (int i = 0; i < evens.length; i = i + 1) {
    print(evens[i].toString());
}

print("Array transform complete");
