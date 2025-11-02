// Passing lambda to generic method test
interface Function<T, R> {
    function apply(T input) : R;
}

class Processor {
    function process<T, R>(T[] items, Function<T, R> mapper) : R[] {
        R[] result = new R[items.length];
        for (int i = 0; i < items.length; i = i + 1) {
            result[i] = mapper.apply(items[i]);
        }
        return result;
    }
}

print("=== Generic Method Inference Test ===");

Processor p = new Processor();
int[] numbers = [1, 2, 3, 4, 5];

// Lambda inferred as Function<int, String>
String[] strings = p.process(numbers, x -> "Value:" + x);
for (int i = 0; i < strings.length; i = i + 1) {
    print(strings[i]);
}

// Lambda inferred as Function<int, int>
int[] doubled = p.process(numbers, x -> x * 2);
for (int i = 0; i < doubled.length; i = i + 1) {
    print("Doubled: " + doubled[i]);
}

print("Generic method inference complete");
