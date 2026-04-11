// MYT-42: box.getClass() on a Box<Int> returns the parameterized Class.
// The canonical name should be "Box<Int>" with MYT-41 spacing.

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/Box.mt";

Box<Int> b = new Box<Int>(42);
Class c = b.getClass();
print("getName: " + c.getName());
print("getRawName: " + c.getRawName());
print("Test passed");
