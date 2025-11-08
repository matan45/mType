// Lambda on empty arrays test
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/Bool.mt";

interface Function<T, R> {
    function apply(T input) : R;
}

class ArrayProcessor {
    public function map(int[] input, Function<Int, Int> mapper) : int[] {
        int[] result = new int[input.length];
        for (int i = 0; i < input.length; i = i + 1) {
            result[i] = mapper.apply(input[i]);
        }
        return result;
    }

    public function filter(int[] input, Function<Int, Bool> predicate) : int[] {
        int count = 0;
        for (int i = 0; i < input.length; i = i + 1) {
            if (predicate.apply(input[i])) {
                count = count + 1;
            }
        }

        int[] result = new int[count];
        int idx = 0;
        for (int i = 0; i < input.length; i = i + 1) {
            if (predicate.apply(input[i])) {
                result[idx] = input[i];
                idx = idx + 1;
            }
        }
        return result;
    }

    public function reduce(int[] input, int initial, Function<Int, Int> accumulator) : int {
        int result = initial;
        for (int i = 0; i < input.length; i = i + 1) {
            result = accumulator.apply(result + input[i]);
        }
        return result;
    }
}

print("=== Zero Length Array Test ===");

ArrayProcessor proc = new ArrayProcessor();

// Map on empty array
int[] empty = new int[0];
int[] mapped = proc.map(empty, x -> x * 2);
print("Mapped empty array length: " + mapped.length);

// Filter on empty array
int[] filtered = proc.filter(empty, x -> x > 0);
print("Filtered empty array length: " + filtered.length);

// Reduce on empty array
int reduced = proc.reduce(empty, 100, x -> x);
print("Reduced empty array: " + reduced);

// Regular array for comparison
int[] normal = [1, 2, 3];
int[] mappedNormal = proc.map(normal, x -> x * 2);
print("Mapped normal array:");
for (int i = 0; i < mappedNormal.length; i = i + 1) {
    print(mappedNormal[i]);
}

// Filter that results in empty array
int[] allFiltered = proc.filter([1, 2, 3], x -> x > 10);
print("All filtered out length: " + allFiltered.length);

// Lambda processing array with single element
int[] single = [42];
int[] mappedSingle = proc.map(single, x -> x + 10);
print("Single element result: " + mappedSingle[0]);

print("Zero length array complete");
