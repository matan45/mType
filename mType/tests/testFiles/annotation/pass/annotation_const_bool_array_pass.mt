// MYT-376: per-element folding inside a bool[] annotation literal, mixing
// relational comparisons and a logical-not constant expression.
import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Annotation.mt";

annotation Flags {
    bool[] values;
}

@Flags(values = [1 < 2, 3 > 4, !false])
class Target { }

Class c = Class::forName("Target");
Annotation? a = c.getAnnotation("Flags");
if (a != null) {
    bool[] v = a.getBoolArray("values");
    for (int i = 0; i < v.length; i = i + 1) {
        print(v[i]);
    }
}

// Expected output:
// true
// false
// true
