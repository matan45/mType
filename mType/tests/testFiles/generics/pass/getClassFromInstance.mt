// MYT-42: obj.getClass() on a non-generic instance returns the runtime Class.

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/primitives/Int.mt";

class Animal {
    public constructor() {}
}

Animal a = new Animal();
Class c = a.getClass();
print("getName: " + c.getName());
print("getRawName: " + c.getRawName());
print("Test passed");
