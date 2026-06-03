// MYT-375: a nullable reference param declared `= null` accepts a non-null
// value; isNull reflects false and the value reads back (complements the
// null/default branch covered by annotation_array_params_pass).
import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Annotation.mt";

@Retention(RUNTIME)
annotation Label {
    string? text = null;
}

@Label(text = "hello")
class Target { }

Class c = Class::forName("Target");
Annotation? l = c.getAnnotation("Label");
if (l != null) {
    print("isNull=" + l.isNull("text"));
    print("text=" + l.getString("text"));
}

// Expected output:
// isNull=false
// text=hello
