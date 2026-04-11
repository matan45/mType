// Test: nested parameterized types compare correctly (Box<Box<Int>>).
// Locks in nested-type handling — string equality over the reconstructed
// full type works because GenericType::toString() and the runtime
// reconstruction both use the same "<", ", ", ">" spacing.

import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Box.mt";

Box<Box<Int>> boxBoxInt = new Box<Box<Int>>(new Box<Int>(new Int(42)));
Box<Box<String>> boxBoxStr = new Box<Box<String>>(new Box<String>(new String("hi")));

print(boxBoxInt isClassOf Box<Box<Int>>);    // true
print(boxBoxInt isClassOf Box<Box<String>>); // false
print(boxBoxStr isClassOf Box<Box<Int>>);    // false
print(boxBoxStr isClassOf Box<Box<String>>); // true
print(boxBoxInt isClassOf Box);              // true (raw fallback)
