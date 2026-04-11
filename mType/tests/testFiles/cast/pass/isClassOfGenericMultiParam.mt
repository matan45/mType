// Test: a class with multiple type parameters — the match respects the
// declared parameter order. Pair<Int, String> and Pair<String, Int> are
// distinct, so swapping the arguments must produce a negative match.

import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

class Pair<K, V> {
    K key;
    V value;
    constructor(K k, V v) {
        this.key = k;
        this.value = v;
    }
}

Pair<Int, String> p1 = new Pair<Int, String>(new Int(1), new String("one"));
Pair<String, Int> p2 = new Pair<String, Int>(new String("two"), new Int(2));

print(p1 isClassOf Pair<Int, String>); // true
print(p1 isClassOf Pair<String, Int>); // false
print(p2 isClassOf Pair<Int, String>); // false
print(p2 isClassOf Pair<String, Int>); // true
print(p1 isClassOf Pair);              // true (raw fallback)
