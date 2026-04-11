// Test: each element returned by getTypeArguments is itself a fully usable
// Class object — you can inspect its name, raw name, and whether it's generic
// or open. This proves getTypeArguments returns real Class instances, not
// a string-typed stub.

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/Box.mt";

Class boxInt = Class::forName("Box<Int>");
Class[] args = boxInt.getTypeArguments();

Class argClass = args[0];
print("Arg getName: " + argClass.getName());
print("Arg getRawName: " + argClass.getRawName());
print("Arg isGeneric: " + argClass.isGeneric());
print("Arg isOpen: " + argClass.isOpen());

print("Test passed");
