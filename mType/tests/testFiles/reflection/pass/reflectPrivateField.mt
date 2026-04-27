// Test: getDeclaredField returns private fields; getField does not include them.
// `getDeclaredField` is class-only (any access); `getField` is public-only inherited view.

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Field.mt";

class Secret {
    private int priv;
    public int pub;

    public constructor() {
        this.priv = 0;
        this.pub = 0;
    }
}

Class c = Class::forName("Secret");

// Declared lookup must surface the private field.
Field privField = c.getDeclaredField("priv");
print("declared priv name: " + privField.getName());

// Public-only enumeration must not include "priv".
Field[] publicFields = c.getFields();
bool seenPriv = false;
bool seenPub = false;
for (Field f : publicFields) {
    if (f.getName() == "priv") {
        seenPriv = true;
    }
    if (f.getName() == "pub") {
        seenPub = true;
    }
}
print("public list saw priv: " + seenPriv);
print("public list saw pub: " + seenPub);

print("Test passed");
