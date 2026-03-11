// Test: ArrayList get() with out of bounds index
// Purpose: Verify IndexOutOfBoundsException is thrown for index >= size

import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";

ArrayList<Int> list = new ArrayList<Int>();
list.add(new Int(10));
list.add(new Int(20));
list.add(new Int(30));

// Index 100 is out of bounds for size 3
Int value = list.get(100);

print("This should not be reached");
