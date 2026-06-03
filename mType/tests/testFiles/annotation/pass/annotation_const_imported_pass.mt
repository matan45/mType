// MYT-376: a `Class::FIELD` constant defined in an IMPORTED file resolves as an
// annotation argument (the resolver recurses into resolved imports before
// serialization).
import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Annotation.mt";
import * from "annotation_const_imported_helper.mt";

annotation Timeout {
    int ms;
}

@Timeout(ms = Cfg::VALUE)
class Service { }

Class c = Class::forName("Service");
Annotation? t = c.getAnnotation("Timeout");
if (t != null) {
    print(t.getInt("ms"));
}
