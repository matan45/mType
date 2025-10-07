import "../../lib/primitives/String.mt";
import "../../lib/primitives/Int.mt";

// Generic function with two type parameters
function <K, V> createPair(K key, V value): void {
    print(key + " = " + value);
}

// Error: Wrong number of type arguments (needs 2, provided 1)
createPair<String>(new String("key"), new Int(42));
