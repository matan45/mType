// MYT-375: an int literal is accepted where a float param is declared and is
// widened to a double (validator isAssignable line 90).
import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Annotation.mt";

@Retention(RUNTIME)
annotation Scale {
    float factor;
}

@Scale(factor = 2)
class Target { }

Class c = Class::forName("Target");
Annotation? s = c.getAnnotation("Scale");
if (s != null) {
    print("factor=" + (s.getFloat("factor") == 2.0));
}

// Expected output:
// factor=true
