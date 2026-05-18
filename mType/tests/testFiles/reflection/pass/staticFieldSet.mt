// Reflectively WRITE a static int field.
// staticFieldAccess.mt only reads via getInt(0); this drives the write side
// through Field.setInt(0, v) -> __reflect_setStaticFieldValue.

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Field.mt";

class Globals {
    public static int counter = 5;
    public static int threshold = 100;
}

print("Direct read before set:");
print("  counter=" + Globals::counter);
print("  threshold=" + Globals::threshold);

Class globalsClass = Class::forName("Globals");
Field counterField = globalsClass.getDeclaredField("counter");
Field thresholdField = globalsClass.getDeclaredField("threshold");

print("isStatic flags:");
print("  counter.isStatic=" + counterField.isStatic());
print("  threshold.isStatic=" + thresholdField.isStatic());

counterField.setInt(0, 99);
thresholdField.setInt(0, 250);

print("After reflective writes - direct read:");
print("  counter=" + Globals::counter);
print("  threshold=" + Globals::threshold);

print("After reflective writes - reflective read:");
print("  counter=" + counterField.getInt(0));
print("  threshold=" + thresholdField.getInt(0));

print("Test passed");
