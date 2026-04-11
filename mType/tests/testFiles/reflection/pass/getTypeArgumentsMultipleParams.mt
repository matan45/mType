// Test: a class with multiple type parameters surfaces its arguments in
// declaration order through getTypeArguments.

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

class Pair<K, V> {
    private K key;
    private V value;

    public constructor(K key, V value) {
        this.key = key;
        this.value = value;
    }
}

Class pairSI = Class::forName("Pair<String, Int>");

Class[] args = pairSI.getTypeArguments();
print("Arg count: " + args.length);
print("Arg 0: " + args[0].getName());
print("Arg 1: " + args[1].getName());

print("Test passed");
