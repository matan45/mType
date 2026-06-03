// MYT-376: a primitive cast constant expression folds in an annotation argument.
import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Annotation.mt";

annotation Cfg {
    int n;
}

@Cfg(n = (int)3.14)
class Target { }

Class c = Class::forName("Target");
Annotation? a = c.getAnnotation("Cfg");
if (a != null) {
    print(a.getInt("n"));
}
