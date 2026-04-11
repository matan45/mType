// Test: Box<Box<Int>> — deeply nested generic of the same template,
// exercising the recursive parser and ensuring inner Box<Int> is still
// a fully usable closed Class reachable via getTypeArguments().

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/Box.mt";

Class boxBoxInt = Class::forName("Box<Box<Int>>");

print("Outer getName: " + boxBoxInt.getName());
print("Outer getRawName: " + boxBoxInt.getRawName());

Class[] outerArgs = boxBoxInt.getTypeArguments();
print("Outer arg count: " + outerArgs.length);

Class innerBox = outerArgs[0];
print("Inner getName: " + innerBox.getName());
print("Inner getRawName: " + innerBox.getRawName());
print("Inner isClosed: " + innerBox.isClosed());

Class[] innerArgs = innerBox.getTypeArguments();
print("Inner arg count: " + innerArgs.length);
print("Innermost getName: " + innerArgs[0].getName());

// Identity: the outer getTypeArguments()[0] should converge with a
// separately-interned Class.forName("Box<Int>") via canonical interning.
Class separatelyInterned = Class::forName("Box<Int>");
print("Inner handle matches forName: " +
      (innerBox.getNativeHandle() == separatelyInterned.getNativeHandle()));

print("Test passed");
