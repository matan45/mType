// MYT-376: constant string concatenation folded in an annotation argument.
import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Annotation.mt";

annotation Tag {
    string name;
}

@Tag(name = "a" + "b")
class Target { }

Class c = Class::forName("Target");
Annotation? t = c.getAnnotation("Tag");
if (t != null) {
    print(t.getString("name"));
}
