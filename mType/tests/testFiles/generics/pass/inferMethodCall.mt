import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Type inference from method arguments
class Pair<K, V> {
    K key;
    V value;

    public function set(K k, V v): void {
        key = k;
        value = v;
    }

    public function getKey(): K {
        return key;
    }

    public function getValue(): V {
        return value;
    }
}

function <K, V> makePair(K k, V v): Pair<K, V> {
    Pair<K, V> p = new Pair<K, V>();
    p.set(k, v);
    return p;
}

function main(): void {
    // Type inference from method arguments
    Pair<Int, String> pair = makePair<Int, String>(new Int(42), new String("answer"));
    print("Key: " + pair.getKey().toString());
    print("Value: " + pair.getValue().toString());
}

main();
