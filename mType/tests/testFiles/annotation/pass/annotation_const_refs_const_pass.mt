// MYT-376: a static-final constant whose initializer references another
// constant (`B = A * 2`) folds via lazy, ordered resolution.
import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Annotation.mt";

annotation Timeout {
    int ms;
}

class Config {
    public static final int A = 10;
    public static final int B = A * 2;
}

@Timeout(ms = Config::B)
class Service { }

Class c = Class::forName("Service");
Annotation? t = c.getAnnotation("Timeout");
if (t != null) {
    print(t.getInt("ms"));
}
