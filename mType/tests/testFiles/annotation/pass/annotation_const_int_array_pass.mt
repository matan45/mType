// MYT-376: per-element folding inside an int[] annotation literal, mixing a
// constant expression, a named constant, and a plain literal.
import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Annotation.mt";

annotation Sizes {
    int[] values;
}

class Config {
    public static final int THIRTY = 30;
}

@Sizes(values = [2 * 10, Config::THIRTY, 40])
class Target { }

Class c = Class::forName("Target");
Annotation? a = c.getAnnotation("Sizes");
if (a != null) {
    int[] v = a.getIntArray("values");
    for (int i = 0; i < v.length; i = i + 1) {
        print(v[i]);
    }
}
