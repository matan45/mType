// Test: Class.forName with a parameterized type whose inner argument refers
// to a class that doesn't exist should throw a clean runtime error naming
// the missing type (not a parse-level failure).

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/primitives/Box.mt";

// NotAType is not a registered class. The base "Box" resolves fine,
// but resolveToReifiedType should fail inside the recursion with a
// "Class not found: NotAType" error.
Class bad = Class::forName("Box<NotAType>");

print("Should not reach here");
