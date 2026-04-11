// Test: an open template Class (forName with no type arguments) reports an
// empty getTypeArguments array even when the class is generic.
// getTypeParameters still returns the parameter names.

import * from "../../lib/reflect/Class.mt";

class Container<T> {
    private T item;

    public constructor(T item) {
        this.item = item;
    }
}

Class containerOpen = Class::forName("Container");

print("isGeneric: " + containerOpen.isGeneric());
print("isOpen: " + containerOpen.isOpen());
print("isClosed: " + containerOpen.isClosed());

string[] params = containerOpen.getTypeParameters();
print("Type param count: " + params.length);

Class[] args = containerOpen.getTypeArguments();
print("Type arg count: " + args.length);

print("Test passed");
