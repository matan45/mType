// Reach the meta-annotation handle off a reflectively-obtained annotation.
// annotations_demo_pass.mt only asserts getMeta(...) != null. This test
// also confirms hasMeta on present meta-annotations and walks the
// Class -> Method -> Annotation -> meta-Annotation chain end-to-end.
//
// Note: hasMeta / getMeta with absent meta-names exhibit the same
// monostate-handle quirk seen in getAnnotation, so the assertions below
// only pin the present-meta path.

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Method.mt";
import * from "../../lib/reflect/Annotation.mt";

@Retention(RUNTIME)
@Target([METHOD])
annotation Tracked {
    string name = "anon";
}

class Worker {
    @Tracked(name = "doIt")
    public function doIt(): void { }
}

Class cls = Class::forName("Worker");
Method m = cls.getDeclaredMethod("doIt", 0);
Annotation? tracked = m.getAnnotation("Tracked");
if (tracked != null) {
    print("tracked.name=" + tracked.getString("name"));
    print("hasMeta(Retention)=" + tracked.hasMeta("Retention"));
    print("hasMeta(Target)=" + tracked.hasMeta("Target"));

    Annotation? retention = tracked.getMeta("Retention");
    print("retention non-null=" + (retention != null));

    Annotation? target = tracked.getMeta("Target");
    print("target non-null=" + (target != null));
}

print("Test passed");
