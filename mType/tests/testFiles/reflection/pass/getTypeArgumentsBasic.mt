// Test: closed Box<Int> exposes a single type argument, and that argument
// reports itself as "Int" through the reflection API.

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/Box.mt";

Class boxInt = Class::forName("Box<Int>");

Class[] args = boxInt.getTypeArguments();
print("Type argument count: " + args.length);
print("First argument name: " + args[0].getName());

print("Test passed");
