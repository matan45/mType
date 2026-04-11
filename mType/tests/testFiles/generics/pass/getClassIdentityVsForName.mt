// MYT-42: box.getClass() and Class.forName("Box<Int>") share the same
// interned handle via ReifiedTypeRegistry.

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/Box.mt";

Box<Int> b = new Box<Int>(7);
Class viaInstance = b.getClass();
Class viaForName = Class::forName("Box<Int>");

print("handles equal: " + (viaInstance.getNativeHandle() == viaForName.getNativeHandle()));
print("names equal: " + (viaInstance.getName() == viaForName.getName()));
print("Test passed");
