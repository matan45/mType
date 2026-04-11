// Test: a closed parameterized Class renders its name in parameterized form,
// while getRawName strips the bindings. Regression gate for Option A —
// the raw name path never picks up type arguments.

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/Box.mt";

Class boxInt = Class::forName("Box<Int>");

print("getName: " + boxInt.getName());
print("getRawName: " + boxInt.getRawName());
print("isGeneric: " + boxInt.isGeneric());
print("isClosed: " + boxInt.isClosed());

Class boxOpen = Class::forName("Box");
print("open getName: " + boxOpen.getName());
print("open getRawName: " + boxOpen.getRawName());
print("open isClosed: " + boxOpen.isClosed());

print("Test passed");
