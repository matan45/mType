// MYT-110: annotations on STATIC method parameters. Static methods have no
// implicit `this` slot, so params[0] is the first user parameter. Also
// exercises Parameter.getAnnotations() (plural) against the
// __reflect_getMethodParameterAnnotations native.

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Method.mt";
import * from "../../lib/reflect/Parameter.mt";
import * from "../../lib/reflect/Annotation.mt";

annotation NotNull { }
annotation Positive { }

class Util {
    public static function clamp(@NotNull @Positive int lo, int hi): int {
        if (lo > hi) {
            return hi;
        }
        return lo;
    }
}

Class c = Class::forName("Util");
Method m = c.getDeclaredMethod("clamp", 2);
Parameter[] params = m.getParameters();

// Static method: params[0] is the first user param (no `this` offset).
print(params[0].getName());
print(params[0].hasAnnotation("NotNull"));
print(params[0].hasAnnotation("Positive"));

// Plural accessor: returns every annotation on params[0].
Annotation[] anns = params[0].getAnnotations();
print(anns.length);

// Second parameter has none.
Annotation[] none = params[1].getAnnotations();
print(none.length);
