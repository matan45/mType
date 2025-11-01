// Test: Error - Using generic type parameters without declaring them
// Expected error: Undefined type 'K' or 'V' or 'T'

class Processor {
    // ERROR: K and V are not declared as type parameters
    // Should be: public function <K, V> createPair(K key, V value): void
    public function createPair(K key, V value): void {
        print("Pair: " + key + " = " + value);
    }

    // ERROR: T is not declared as type parameter
    // Should be: public function <T> createPair2(T key): T
    public function createPair2(T key): T {
        return key;
    }
}

function main(): void {
    Processor proc = new Processor();
    // This should fail during compilation
}

main();
