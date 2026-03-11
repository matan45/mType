// Test: HashMap entry iterator on empty map
// Purpose: Verify exception is thrown when calling next() on empty map iterator

import * from "../../lib/collections/HashMap.mt";
import * from "../../lib/interfaces/MapEntry.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/Iterator.mt";

HashMap<String, Int> map = new HashMap<String, Int>();

Iterator<MapEntry<String, Int>> iter = map.entryIterator();

// This should throw immediately - empty map has no elements
iter.next();

print("This should not be reached");
