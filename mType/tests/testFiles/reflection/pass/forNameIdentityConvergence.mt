// Test: two Class.forName calls for the same canonical parameterized type
// converge to the same handle (interning through ReifiedTypeRegistry).
// Spaces inside the type argument list don't break identity because both
// paths funnel through the same canonical-string key.

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/Box.mt";

Class a = Class::forName("Box<Int>");
Class b = Class::forName("Box<Int>");
Class c = Class::forName("Box< Int >");

print("a == b handle: " + (a.getNativeHandle() == b.getNativeHandle()));
print("a == c handle: " + (a.getNativeHandle() == c.getNativeHandle()));
print("a getName: " + a.getName());
print("c getName: " + c.getName());

print("Test passed");
