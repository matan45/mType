// Test: HashMap value iterator past end
// Purpose: Verify exception is thrown when calling next() past the last value

import * from "../../lib/collections/HashMap.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/Iterator.mt";

HashMap<String, Int> map = new HashMap<String, Int>();
map.put(new String("x"), new Int(10));
map.put(new String("y"), new Int(20));

Iterator<Int> iter = map.valueIterator();

// Iterate through all values
while (iter.hasNext()) {
    Int value = iter.next();
}

// This should throw - no more elements
iter.next();

print("This should not be reached");
