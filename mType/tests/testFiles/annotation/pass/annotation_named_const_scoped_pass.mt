// MYT-376: a `Class::FIELD` static-final named constant resolves as an
// annotation argument.
import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Annotation.mt";

annotation Timeout {
    int ms;
}

class Config {
    public static final int DEFAULT_MS = 250;
}

@Timeout(ms = Config::DEFAULT_MS)
class Service { }

Class c = Class::forName("Service");
Annotation? t = c.getAnnotation("Timeout");
if (t != null) {
    print(t.getInt("ms"));
}
