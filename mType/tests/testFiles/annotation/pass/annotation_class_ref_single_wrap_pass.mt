// MYT-375: a single Class value is accepted where Class[] is declared and
// wrapped into a one-element array (validator isAssignable + coerceValue).
import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Annotation.mt";

class Handler { }

@Retention(RUNTIME)
annotation Uses {
    Class[] types;
}

@Uses(types = Handler)
class Target { }

Class c = Class::forName("Target");
Annotation? u = c.getAnnotation("Uses");
if (u != null) {
    string[] names = u.getClassNames("types");
    print("len=" + names.length);
    print("first=" + names[0]);
}

// Expected output:
// len=1
// first=Handler
