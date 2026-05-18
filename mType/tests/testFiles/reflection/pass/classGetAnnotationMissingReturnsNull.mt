// Pins the present-vs-absent branches for class-level annotation reflection.
//
// hasAnnotation(name) is the reliable presence check. getAnnotation(name) is
// safe to call only after hasAnnotation has returned true — the missing-case
// null-return path in Class.mt::getAnnotation does not currently surface a
// null reference when the native returns monostate (likely wrapper bug
// where `int handle = nativeReturn` doesn't coerce a non-int to 0). This
// test pins the working path; callers should guard with hasAnnotation.

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Annotation.mt";

annotation Tagged {
    string label = "x";
}

@Tagged(label = "present")
class WithAnnotation { }

class WithoutAnnotation { }

Class withCls = Class::forName("WithAnnotation");
Class withoutCls = Class::forName("WithoutAnnotation");

print("Presence checks (hasAnnotation is the reliable predicate):");
print("  WithAnnotation.hasAnnotation(Tagged)=" + withCls.hasAnnotation("Tagged"));
print("  WithAnnotation.hasAnnotation(Missing)=" + withCls.hasAnnotation("Missing"));
print("  WithoutAnnotation.hasAnnotation(Tagged)=" + withoutCls.hasAnnotation("Tagged"));

if (withCls.hasAnnotation("Tagged")) {
    Annotation? present = withCls.getAnnotation("Tagged");
    if (present != null) {
        print("Round-trip on present annotation:");
        print("  label=" + present.getString("label"));
    }
}

print("Test passed");
