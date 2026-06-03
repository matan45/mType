// MYT-376: unary, logical-not, and ternary constant expressions fold in
// annotation arguments.
import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Annotation.mt";

annotation Cfg {
    int n;
    bool flag;
    int pick;
}

@Cfg(n = -5, flag = !false, pick = true ? 7 : 9)
class Target { }

Class c = Class::forName("Target");
Annotation? a = c.getAnnotation("Cfg");
if (a != null) {
    print(a.getInt("n"));
    print("flag=" + a.getBool("flag"));
    print(a.getInt("pick"));
}
