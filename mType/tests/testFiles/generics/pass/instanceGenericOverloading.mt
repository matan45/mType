import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";

// Test: Multiple generic instance methods with different type parameter counts
class Processor {
    // Single type parameter
    public function <T> processSingle(T value): void {
        print("Single: " + value);
    }

    // Two type parameters
    public function <T, U> processPair(T first, U second): void {
        print("Pair: " + first + ", " + second);
    }
}

function main(): void {
    Processor proc = new Processor();

    // Call single-parameter version
    proc.processSingle<String>(new String("hello"));
    proc.processSingle<Int>(new Int(42));

    // Call two-parameter version
    proc.processPair<String, Int>(new String("count"), new Int(10));
    proc.processPair<Int, String>(new Int(1), new String("one"));

    print("Generic method overloading test passed");
}

main();
