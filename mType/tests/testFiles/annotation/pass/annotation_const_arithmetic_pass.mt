// MYT-376: a constant arithmetic expression is folded to a literal in an
// annotation argument and round-trips through reflection.
import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Annotation.mt";

annotation Timeout {
    int ms;
}

@Timeout(ms = 60 * 1000)
class Service { }

Class c = Class::forName("Service");
Annotation? t = c.getAnnotation("Timeout");
if (t != null) {
    print(t.getInt("ms"));
}
