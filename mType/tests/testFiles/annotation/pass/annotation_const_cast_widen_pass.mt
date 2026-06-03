// MYT-376: constant casts to the non-int primitive targets (float, String,
// bool) fold in annotation arguments — the (int) cast is already covered.
// The float is compared against a literal to avoid float print-format coupling
// (mirrors annotation_const_expr_float_widen_pass).
import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Annotation.mt";

annotation Cfg {
    float f;
    string label;
    bool b;
}

@Cfg(f = (float)7, label = (String)42, b = (bool)1)
class Target { }

Class c = Class::forName("Target");
Annotation? a = c.getAnnotation("Cfg");
if (a != null) {
    print("f=" + (a.getFloat("f") == 7.0));
    print(a.getString("label"));
    print(a.getBool("b"));
}

// Expected output:
// f=true
// 42
// true
