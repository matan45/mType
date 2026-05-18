// DOCUMENTATION PIN: __reflect_setFieldValue does NOT type-check before
// delegating to instance->setField. Writing a wrong-typed Object into an
// int field silently succeeds, and the field afterwards holds the wrong
// value (prints as <object>). Filed for follow-up — pin the current
// behavior here so a future fix changes this test deliberately rather
// than catching us by surprise.
//
// See: mType/reflection/ReflectionNatives_Field.cpp __reflect_setFieldValue,
// which does not inspect fieldInfo.field->getDeclaredType() before the
// instance->setField call.

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Field.mt";

class Tag {
    public string label;
    public constructor() {
        this.label = "x";
    }
}

class Box {
    public int n;
    public constructor() {
        this.n = 0;
    }
}

Box b = new Box();
Class cls = Class::forName("Box");
Field nField = cls.getDeclaredField("n");

print("before wrong-type set, n=" + b.n);
Tag wrong = new Tag();
nField.set(b, wrong);
print("after wrong-type set, n=" + b.n);
print("Test passed");
