// Test: For-each loop with incompatible element type
// Expected: Type error - loop variable type incompatible with iterator element type

import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";

// Create an ArrayList<String>
ArrayList<String> stringList = new ArrayList<String>();
stringList.add(new String("hello"));
stringList.add(new String("world"));

// This should fail: trying to iterate String collection with Int variable
for (Int x : stringList) {
    print(x);
}
