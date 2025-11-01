import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";

// Test: Overloaded generic instance methods
class Processor {
    // Single parameter
    public function <T> process(T value): void {
        print("Single: " + value);
    }

    // Two parameters
    public function <T, U> process(T first, U second): void {
        print("Pair: " + first + ", " + second);
    }
}

function main(): void {
    Processor proc = new Processor();

    // Call single-parameter version
    proc.process<String>(new String("hello"));
    proc.process<Int>(new Int(42));

    // Call two-parameter version
    proc.process<String, Int>(new String("count"), new Int(10));
    proc.process<Int, String>(new Int(1), new String("one"));

    print("Generic method overloading test passed");
}

main();
