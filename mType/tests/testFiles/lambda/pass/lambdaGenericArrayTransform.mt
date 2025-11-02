// Transform generic arrays with lambdas
interface Function<T, R> {
    function apply(T input) : R;
}

class ArrayUtils {
    function map<T, R>(T[] input, Function<T, R> transformer) : R[] {
        R[] result = new R[input.length];
        for (int i = 0; i < input.length; i = i + 1) {
            result[i] = transformer.apply(input[i]);
        }
        return result;
    }

    function filter<T>(T[] input, Function<T, bool> predicate) : T[] {
        int count = 0;
        for (int i = 0; i < input.length; i = i + 1) {
            if (predicate.apply(input[i])) {
                count = count + 1;
            }
        }

        T[] result = new T[count];
        int idx = 0;
        for (int i = 0; i < input.length; i = i + 1) {
            if (predicate.apply(input[i])) {
                result[idx] = input[i];
                idx = idx + 1;
            }
        }
        return result;
    }
}

print("=== Generic Array Transform Test ===");

ArrayUtils utils = new ArrayUtils();
int[] numbers = [1, 2, 3, 4, 5];

// Map int[] to String[]
String[] strings = utils.map(numbers, x -> "Item-" + x);
for (int i = 0; i < strings.length; i = i + 1) {
    print(strings[i]);
}

// Filter even numbers
int[] evens = utils.filter(numbers, x -> x % 2 == 0);
print("Evens:");
for (int i = 0; i < evens.length; i = i + 1) {
    print(evens[i]);
}

print("Array transform complete");
