// Test: non-generic classes report empty getTypeArguments and empty
// getTypeParameters.

import * from "../../lib/reflect/Class.mt";

class Plain {
    public int x;
}

Class plainClass = Class::forName("Plain");

print("isGeneric: " + plainClass.isGeneric());

string[] params = plainClass.getTypeParameters();
print("Type param count: " + params.length);

Class[] args = plainClass.getTypeArguments();
print("Type arg count: " + args.length);

print("Test passed");
