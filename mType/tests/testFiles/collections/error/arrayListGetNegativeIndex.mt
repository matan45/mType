// Test: ArrayList get() with negative index
// Purpose: Verify IndexOutOfBoundsException is thrown for negative index

import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";

ArrayList<Int> list = new ArrayList<Int>();
list.add(new Int(10));
list.add(new Int(20));
list.add(new Int(30));

// Negative index is out of bounds
Int value = list.get(-1);

print("This should not be reached");
