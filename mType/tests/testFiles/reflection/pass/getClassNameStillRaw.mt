// Regression gate for Option A: the raw-name path must stay raw, even
// when the same class is also reached through a parameterized handle.
// ValueObject::getClassName() (and every caller of it — see the bytecode
// VM, ObjectExecutor, and JIT helpers) must keep returning "Box", not
// "Box<Int>", so the reflection Class.getRawName() path acts as the
// contract surface.

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/Box.mt";

// Both open and closed handles must agree on the raw name.
Class boxOpen = Class::forName("Box");
Class boxInt = Class::forName("Box<Int>");

print("open getRawName: " + boxOpen.getRawName());
print("closed getRawName: " + boxInt.getRawName());
print("raw names match: " + (boxOpen.getRawName() == boxInt.getRawName()));

// And the closed handle's getName() must render the parameterized form
// precisely because the raw path does NOT.
print("closed getName: " + boxInt.getName());
print("names differ: " + (boxInt.getName() != boxInt.getRawName()));

print("Test passed");
