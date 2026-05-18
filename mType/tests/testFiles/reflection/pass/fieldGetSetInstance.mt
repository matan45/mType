// Field.get(Object) / Field.set(Object, Object) round-trip on instance fields.
// Uses object-typed fields so primitives don't need autoboxing into Object,
// which mType doesn't perform at the call site (MT-E2007).
//
// Closes the gap left by reflectionInvocation.mt (which only exercises invoke)
// and staticFieldAccess.mt (which only reads).

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Field.mt";

class Tag {
    public string text;
    public constructor(string s) {
        this.text = s;
    }
}

class Container {
    public Tag primary;
    public Tag secondary;

    public constructor() {
        this.primary = new Tag("alpha");
        this.secondary = new Tag("beta");
    }
}

Container c = new Container();
print("Before reflective access:");
print("  primary.text=" + c.primary.text);
print("  secondary.text=" + c.secondary.text);

Class containerClass = Class::forName("Container");
Field primaryField = containerClass.getDeclaredField("primary");
Field secondaryField = containerClass.getDeclaredField("secondary");

Object readPrimary = primaryField.get(c);
Object readSecondary = secondaryField.get(c);
print("Reflective get returned non-null:");
print("  primary non-null=" + (readPrimary != null));
print("  secondary non-null=" + (readSecondary != null));

Tag newTag = new Tag("rewritten");
primaryField.set(c, newTag);
secondaryField.set(c, newTag);

print("After Field.set:");
print("  primary.text=" + c.primary.text);
print("  secondary.text=" + c.secondary.text);
print("  same instance after both sets=" + (c.primary == c.secondary));

print("Test passed");
