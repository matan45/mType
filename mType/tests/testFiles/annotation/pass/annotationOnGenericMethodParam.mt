// Test: an annotation applied to a parameter of a generic method survives
// through compilation and is reflectable. Verifies that generic type
// parameters and parameter-target annotations cohabit on the same method.

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Method.mt";
import * from "../../lib/reflect/Parameter.mt";
import * from "../../lib/reflect/Annotation.mt";

@Retention(RUNTIME)
@Target([PARAMETER])
annotation Range {
    int min = 0;
    int max = 100;
}

class Validator {
    public function <T> check(@Range(min = 1, max = 10) T value): void {
    }
}

Class c = Class::forName("Validator");
Method m = c.getDeclaredMethod("check", 1);
Parameter[] params = m.getParameters();

// Instance methods prepend `this` at index 0; the user parameter is at index 1.
print(params[1].getName());
print(params[1].hasAnnotation("Range"));

Annotation? a = params[1].getAnnotation("Range");
if (a != null) {
    print(a.getInt("min"));
    print(a.getInt("max"));
}
