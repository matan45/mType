// Test: ArrayList get() on empty list
// Purpose: Verify IndexOutOfBoundsException is thrown for any index on empty list

import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";

ArrayList<Int> list = new ArrayList<Int>();

// Index 0 is out of bounds for empty list (size 0)
Int value = list.get(0);

print("This should not be reached");
