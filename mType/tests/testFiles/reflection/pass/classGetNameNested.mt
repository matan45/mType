// Test: nested parameterized types round-trip through getName, and the
// inner Class object (reached via getTypeArguments) itself renders in
// canonical parameterized form.

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Box.mt";

class Pair<K, V> {
    private K key;
    private V value;

    public constructor(K key, V value) {
        this.key = key;
        this.value = value;
    }
}

Class nested = Class::forName("Pair<String, Box<Int>>");

print("Outer getName: " + nested.getName());
print("Outer getRawName: " + nested.getRawName());

Class[] args = nested.getTypeArguments();
print("Outer arg count: " + args.length);
print("Arg 0 getName: " + args[0].getName());
print("Arg 1 getName: " + args[1].getName());
print("Arg 1 getRawName: " + args[1].getRawName());

Class[] innerArgs = args[1].getTypeArguments();
print("Inner arg count: " + innerArgs.length);
print("Inner arg 0 getName: " + innerArgs[0].getName());

print("Test passed");
