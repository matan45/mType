// MYT-367 regression: multi-arg parameterized-interface cast. Validates
// that splitTopLevelTypeArgs + rebindForInterface handle more than one
// type parameter when substituting bindings on the cast path. Single-T
// fixtures pass even if multi-arg arity handling regresses.

import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

interface IPair<K, V> {
    public function key(): K;
    public function val(): V;
}

class Pair<K, V> implements IPair<K, V> {
    public K k;
    public V v;
    public constructor(K k, V v) { this.k = k; this.v = v; }
    public function key(): K { return this.k; }
    public function val(): V { return this.v; }
}

Pair<Int, String> p = new Pair<Int, String>(new Int(7), new String("seven"));
IPair<Int, String> asIface = (IPair<Int, String>)p;
print(asIface.key().toString());
print(asIface.val().toString());

// Expected output:
// 7
// seven
