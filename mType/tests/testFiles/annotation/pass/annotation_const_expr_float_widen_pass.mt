// MYT-376: a constant expression folds to int 20, then the validator widens it
// to the declared float parameter (isAssignable int -> float).
import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Annotation.mt";

annotation Scale {
    float factor;
}

@Scale(factor = 2 * 10)
class Target { }

Class c = Class::forName("Target");
Annotation? s = c.getAnnotation("Scale");
if (s != null) {
    print("factor=" + (s.getFloat("factor") == 20.0));
}
