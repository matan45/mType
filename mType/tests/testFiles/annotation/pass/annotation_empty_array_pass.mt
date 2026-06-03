// MYT-375: an empty array literal [] is accepted for any array-typed param and
// coerced to a typed empty array from the declared element type (coerceValue).
import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Annotation.mt";

@Retention(RUNTIME)
annotation Names {
    string[] values;
}

@Names(values = [])
class Target { }

Class c = Class::forName("Target");
Annotation? n = c.getAnnotation("Names");
if (n != null) {
    string[] v = n.getStringArray("values");
    print("len=" + v.length);
}

// Expected output:
// len=0
