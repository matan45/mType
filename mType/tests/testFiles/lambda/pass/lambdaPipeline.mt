// Stream-like lambda pipeline test
import * from "../../../lib/primitives/Int.mt";
import * from "../../../lib/primitives/Bool.mt";

interface Function<T, R> {
    function apply(T input) : R;
}

class Pipeline<T> {
    T value;

    function init(T val) {
        this.value = val;
    }

    function map<R>(Function<T, R> mapper) : Pipeline<R> {
        R newValue = mapper.apply(this.value);
        return new Pipeline<R>(newValue);
    }

    function get() : T {
        return this.value;
    }
}

class ArrayPipeline {
    int[] data;

    function init(int[] arr) {
        this.data = arr;
    }

    function map(Function<int, int> mapper) : ArrayPipeline {
        int[] result = new int[this.data.length];
        for (int i = 0; i < this.data.length; i = i + 1) {
            result[i] = mapper.apply(this.data[i]);
        }
        return new ArrayPipeline(result);
    }

    function filter(Function<int, bool> predicate) : ArrayPipeline {
        int count = 0;
        for (int i = 0; i < this.data.length; i = i + 1) {
            if (predicate.apply(this.data[i])) {
                count = count + 1;
            }
        }

        int[] result = new int[count];
        int idx = 0;
        for (int i = 0; i < this.data.length; i = i + 1) {
            if (predicate.apply(this.data[i])) {
                result[idx] = this.data[i];
                idx = idx + 1;
            }
        }
        return new ArrayPipeline(result);
    }

    function sum() : int {
        int total = 0;
        for (int i = 0; i < this.data.length; i = i + 1) {
            total = total + this.data[i];
        }
        return total;
    }
}

print("=== Pipeline Test ===");

// Simple value pipeline
Pipeline<Int> p1 = new Pipeline<Int>(new Int(10));
Pipeline<Int> p2 = p1.map(x -> new Int(x.getValue() * 2));
Pipeline<Int> p3 = p2.map(x -> new Int(x.getValue() + 5));

print("Pipeline result: " + p3.get().getValue());

// Chained pipeline
Pipeline<Int> result = new Pipeline<Int>(new Int(5))
    .map(x -> new Int(x.getValue() * 3))
    .map(x -> new Int(x.getValue() + 10))
    .map(x -> new Int(x.getValue() * 2));

print("Chained result: " + result.get().getValue());

// Array pipeline
int[] numbers = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
ArrayPipeline ap = new ArrayPipeline(numbers);

int sumOfDoubledEvens = ap
    .filter(x -> x % 2 == 0)
    .map(x -> x * 2)
    .sum();

print("Sum of doubled evens: " + sumOfDoubledEvens);

// Complex pipeline
int complexResult = new ArrayPipeline([1, 2, 3, 4, 5])
    .map(x -> x * x)
    .filter(x -> x > 5)
    .map(x -> x + 1)
    .sum();

print("Complex pipeline: " + complexResult);

print("Pipeline complete");
