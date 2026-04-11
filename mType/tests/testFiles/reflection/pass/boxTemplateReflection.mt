// Test: the open Box template (no type arguments) reports its single
// type parameter "T" via getTypeParameters, and reports an empty
// getTypeArguments. Also verifies that the open-form template Class
// is itself reflectable — we can enumerate the Box methods.

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/primitives/Box.mt";

Class boxOpen = Class::forName("Box");

print("isGeneric: " + boxOpen.isGeneric());
print("isOpen: " + boxOpen.isOpen());
print("isClosed: " + boxOpen.isClosed());

string[] params = boxOpen.getTypeParameters();
print("Type param count: " + params.length);
print("Type param 0: " + params[0]);

Class[] args = boxOpen.getTypeArguments();
print("Type arg count: " + args.length);

// Template-level reflection should still expose Box's methods — every
// existing native keeps working against a closed handle OR an open one.
Method[] methods = boxOpen.getMethods();
print("Method count > 0: " + (methods.length > 0));

print("Test passed");
