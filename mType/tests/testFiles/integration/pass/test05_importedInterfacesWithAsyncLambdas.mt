// Integration Test 05: Imported Interfaces with Async Lambdas
// Tests: Imports + Interfaces + Async lambdas + Generic methods

import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/collections/List.mt";

// Async functional interfaces
interface AsyncFunction {
    function async apply(int x): Promise<Int>;
}

interface AsyncTransformer<T, R> {
    function async transform(T input): Promise<R>;
}

// Implementations with async lambdas
class AsyncDoubler implements AsyncFunction {
    public function async apply(int x): Promise<Int> {
        int result = x * 2;
        return new Int(result);
    }
}

class AsyncAdder implements AsyncFunction {
    private int addValue;

    constructor(int val) {
        this.addValue = val;
    }

    public function async apply(int x): Promise<Int> {
        int result = x + this.addValue;
        return new Int(result);
    }
}

class IntToStringTransformer implements AsyncTransformer<Int, String> {
    public function async transform(Int input): Promise<String> {
        string result = "Value: " + input.getValue();
        return new String(result);
    }
}

class StringToIntTransformer implements AsyncTransformer<String, Int> {
    public function async transform(String input): Promise<Int> {
        // Simulate parsing length
        int length = strLength(input.getValue());
        return new Int(length);
    }
}

// Generic async processor
class AsyncProcessor<T, R> {
    private AsyncTransformer<T, R> transformer;

    constructor(AsyncTransformer<T, R> trans) {
        this.transformer = trans;
    }

    public function async process(T input): Promise<R> {
        R result = await this.transformer.transform(input);
        return result;
    }
}

// Test with collections
class AsyncListProcessor {
    private List<Int> numbers;

    constructor() {
        this.numbers = new List<Int>();
    }

    public function addNumber(Int num): void {
        this.numbers.add(num);
    }

    public function async mapAsync(AsyncFunction func): Promise<List<Int>> {
        List<Int> result = new List<Int>();
        for (int i = 0; i < this.numbers.size(); i = i + 1) {
            Int current = this.numbers.get(i);
            Int mapped = await func.apply(current.getValue());
            result.add(mapped);
        }
        return result;
    }
}

// Global async generic function
function async <T, R> asyncTransform(T value, AsyncTransformer<T, R> transformer): Promise<R> {
    R result = await transformer.transform(value);
    return result;
}

// Main test execution
function async main(): Promise<void> {
    print("=== Test 05: Imported Interfaces with Async Lambdas ===");

    // Test 1: Basic async function
    print("--- Basic async function ---");
    AsyncFunction doubler = new AsyncDoubler();
    Int result1 = await doubler.apply(21);
    print("Double 21: " + result1.getValue());

    AsyncFunction adder = new AsyncAdder(100);
    Int result2 = await adder.apply(50);
    print("Add 100 to 50: " + result2.getValue());

    // Test 2: Async transformer with generics
    print("--- Async transformers ---");
    IntToStringTransformer intToStr = new IntToStringTransformer();
    AsyncProcessor<Int, String> processor1 = new AsyncProcessor<Int, String>(intToStr);

    String strResult = await processor1.process(new Int(42));
    print(strResult.getValue());

    StringToIntTransformer strToInt = new StringToIntTransformer();
    AsyncProcessor<String, Int> processor2 = new AsyncProcessor<String, Int>(strToInt);

    Int intResult = await processor2.process(new String("Hello World"));
    print("String length: " + intResult.getValue());

    // Test 3: Async list processing
    print("--- Async list processing ---");
    AsyncListProcessor listProcessor = new AsyncListProcessor();
    listProcessor.addNumber(new Int(5));
    listProcessor.addNumber(new Int(10));
    listProcessor.addNumber(new Int(15));

    AsyncFunction multiplier = new AsyncAdder(10);
    List<Int> mappedList = await listProcessor.mapAsync(multiplier);

    print("Mapped list:");
    for (int i = 0; i < mappedList.size(); i++) {
        Int val = mappedList.get(i);
        print(val.getValue());
    }

    // Test 4: Global generic async function
    print("--- Global generic async function ---");
    Int value = new Int(99);
    String transformed1 = await asyncTransform<Int, String>(value, intToStr);
    print(transformed1.getValue());

    String str = new String("Testing");
    Int transformed2 = await asyncTransform<String, Int>(str, strToInt);
    print("Transformed string length: " + transformed2.getValue());

    // Test 5: Chained async operations
    print("--- Chained async operations ---");
    Int initial = new Int(10);
    String step1 = await asyncTransform<Int, String>(initial, intToStr);
    print("Step 1: " + step1.getValue());

    Int step2 = await asyncTransform<String, Int>(step1, strToInt);
    print("Step 2: " + step2.getValue());

    // Test 6: Multiple async calls in sequence
    print("--- Multiple async calls ---");
    AsyncFunction func1 = new AsyncDoubler();
    AsyncFunction func2 = new AsyncAdder(5);

    Int seq1 = await func1.apply(10);
    print("First operation: " + seq1.getValue());

    Int seq2 = await func2.apply(seq1.getValue());
    print("Second operation: " + seq2.getValue());

    print("=== Test 05 Complete ===");
    return null;
}

main();
