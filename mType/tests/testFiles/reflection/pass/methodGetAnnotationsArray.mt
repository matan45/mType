// Drive __reflect_getMethodAnnotations (the array-form native).
// Existing tests only call Method.getAnnotation(name) (singular). This pins
// the bulk enumeration path: native returns int[] of annotation handles
// which the caller wraps in Annotation. Method.mt has no getAnnotations()
// wrapper yet, so this test exercises the native directly.

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Method.mt";
import * from "../../lib/reflect/Annotation.mt";

annotation Logged {
    string level = "info";
}

annotation Cached {
    int ttl = 60;
}

class Service {
    @Logged(level = "debug")
    @Cached(ttl = 120)
    public function hot(): void { }

    public function plain(): void { }
}

Class svc = Class::forName("Service");

Method hot = svc.getDeclaredMethod("hot", 0);
int[] hotHandles = __reflect_getMethodAnnotations(hot.getNativeHandle());
print("hot annotation handle count=" + hotHandles.length);

Method plain = svc.getDeclaredMethod("plain", 0);
int[] plainHandles = __reflect_getMethodAnnotations(plain.getNativeHandle());
print("plain annotation handle count=" + plainHandles.length);

print("hot.hasAnnotation(Logged)=" + hot.hasAnnotation("Logged"));
print("hot.hasAnnotation(Cached)=" + hot.hasAnnotation("Cached"));
print("hot.hasAnnotation(Missing)=" + hot.hasAnnotation("Missing"));

Annotation? logged = hot.getAnnotation("Logged");
if (logged != null) {
    print("logged.level=" + logged.getString("level"));
}
Annotation? cached = hot.getAnnotation("Cached");
if (cached != null) {
    print("cached.ttl=" + cached.getInt("ttl"));
}

print("Test passed");
