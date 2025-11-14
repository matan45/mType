// Test: For-each loop with primitive type mismatch
// Expected: Type error - primitive int cannot be used with String collection

import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/String.mt";

ArrayList<String> strings = new ArrayList<String>();
strings.add(new String("test"));

// This should fail: primitive int vs String object
for (int x : strings) {
    print(x);
}
