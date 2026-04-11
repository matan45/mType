// Test: isClassOf with a closed parameterized RHS must discriminate between
// different type arguments on the same base class. This is the headline
// bug-fix for MYT-41 requirement 1 — prior to this change Box<Int> and
// Box<String> both satisfied `box isClassOf Box<Int>`.

import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Box.mt";

Box<Int> boxInt = new Box<Int>(new Int(42));
Box<String> boxStr = new Box<String>(new String("hello"));

print(boxInt isClassOf Box<Int>);    // true
print(boxInt isClassOf Box<String>); // false
print(boxStr isClassOf Box<Int>);    // false
print(boxStr isClassOf Box<String>); // true
