import * from "../../lib/primitives/String.mt";

// Test: Error - Wrong number of type arguments
// Expected error: Method expects 2 type arguments, but got 1

class Processor {
    public function <K, V> createPair(K key, V value): void {
        print("Pair: " + key + " = " + value);
    }
}

function main(): void {
    Processor proc = new Processor();

    // ERROR: Method expects 2 type arguments <K, V>, but only 1 provided
    proc.createPair<String>(new String("key"), new String("value"));

    print("This should not execute");
}

main();
