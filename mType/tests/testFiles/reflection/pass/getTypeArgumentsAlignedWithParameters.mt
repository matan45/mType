// Test: getTypeArguments()[i] lines up positionally with getTypeParameters()[i].
// Print each "K=String" pair to verify declaration-order alignment.

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

string[] params = pairSI.getTypeParameters();
Class[] args = pairSI.getTypeArguments();

print("Params length: " + params.length);
print("Args length: " + args.length);

for (int i = 0; i < params.length; i = i + 1) {
    print("  " + params[i] + "=" + args[i].getName());
}

print("Test passed");
