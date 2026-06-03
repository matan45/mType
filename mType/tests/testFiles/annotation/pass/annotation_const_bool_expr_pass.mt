// MYT-376: a bool annotation parameter folded from comparison and logical
// (&&, ||) constant expressions — not just the unary `!` / ternary forms.
import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Annotation.mt";

annotation Cfg {
    bool on;
    bool off;
}

@Cfg(on = 10 > 3 && 2 == 2, off = 5 <= 4 || false)
class Target { }

Class c = Class::forName("Target");
Annotation? a = c.getAnnotation("Cfg");
if (a != null) {
    print(a.getBool("on"));
    print(a.getBool("off"));
}

// Expected output:
// true
// false
