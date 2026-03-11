// Test: HashMap entry iterator past end
// Purpose: Verify exception is thrown when calling next() past the last entry

import * from "../../lib/collections/HashMap.mt";
import * from "../../lib/interfaces/MapEntry.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/Iterator.mt";

HashMap<String, Int> map = new HashMap<String, Int>();
map.put(new String("a"), new Int(1));
map.put(new String("b"), new Int(2));

Iterator<MapEntry<String, Int>> iter = map.entryIterator();

// Iterate through all entries
while (iter.hasNext()) {
    MapEntry<String, Int> entry = iter.next();
}

// This should throw - no more elements
iter.next();

print("This should not be reached");
