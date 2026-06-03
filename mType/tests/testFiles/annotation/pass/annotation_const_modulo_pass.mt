// MYT-376: the modulo (%) operator folds in an annotation argument.
import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Annotation.mt";

annotation Cfg {
    int x;
}

@Cfg(x = 17 % 5)
class Target { }

Class c = Class::forName("Target");
Annotation? a = c.getAnnotation("Cfg");
if (a != null) {
    print(a.getInt("x"));
}

// Expected output:
// 2
