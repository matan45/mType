import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";

// Test: Instance generic method with multiple type parameters
class Processor {
    public function <K, V> createPair(K key, V value): void {
        print("Pair: " + key + " = " + value);
    }

    public function <A, B, C> createTriple(A first, B second, C third): void {
        print("Triple: " + first + ", " + second + ", " + third);
    }
}

function main(): void {
    Processor proc = new Processor();

    // Test with two type parameters
    proc.createPair<String, Int>(new String("age"), new Int(25));
    proc.createPair<Int, String>(new Int(1), new String("one"));

    // Test with three type parameters
    proc.createTriple<String, Int, String>(
        new String("name"),
        new Int(42),
        new String("value")
    );

    print("Multiple type parameters test passed");
}

main();
