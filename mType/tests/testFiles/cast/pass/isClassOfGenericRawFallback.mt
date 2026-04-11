// Test: closed parameterized instances still satisfy `isClassOf BaseRaw`
// (type-erased backward compatibility). This is the regression gate that
// keeps the existing isClassOfGeneric.mt behavior — a Box<Int> instance
// is still considered an instance of the raw Box template.

import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Box.mt";

Box<Int> boxInt = new Box<Int>(new Int(42));
Box<String> boxStr = new Box<String>(new String("hello"));

print(boxInt isClassOf Box); // true
print(boxStr isClassOf Box); // true
