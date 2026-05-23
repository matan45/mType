// MYT-360: primitive type rejected when mixed with a boxed type in
// `implements Map<Boxed, primitive>`. The boxed first arg should not mask
// the primitive second arg.

import * from "../../lib/primitives/Int.mt";

interface Map<K, V> {
    function put(K key, V value): void;
}

class MyMap implements Map<Int, int> {
    public function put(Int key, int value): void {
        print(key);
        print(value);
    }
}

function main(): void {
    print("unreachable - parse should fail");
}

main();
