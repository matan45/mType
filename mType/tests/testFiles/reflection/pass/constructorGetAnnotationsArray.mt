// Constructor.getAnnotations() (array form) is untested elsewhere.
// annotation_on_ctor_reflection_pass.mt only drives hasAnnotation +
// getAnnotation(name). Pins __reflect_getConstructorAnnotations.

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Constructor.mt";
import * from "../../lib/reflect/Annotation.mt";

annotation Inject { }

annotation Wired {
    string scope = "singleton";
}

class Service {
    @Inject
    @Wired(scope = "request")
    public constructor() { }
}

class Plain {
    public constructor() { }
}

Class svc = Class::forName("Service");
Constructor svcCtor = svc.getDeclaredConstructor(0);
Annotation[] svcAnnos = svcCtor.getAnnotations();
print("Service ctor annotation count=" + svcAnnos.length);

Class plain = Class::forName("Plain");
Constructor plainCtor = plain.getDeclaredConstructor(0);
Annotation[] plainAnnos = plainCtor.getAnnotations();
print("Plain ctor annotation count=" + plainAnnos.length);

print("Service ctor.hasAnnotation(Inject)=" + svcCtor.hasAnnotation("Inject"));
print("Service ctor.hasAnnotation(Wired)=" + svcCtor.hasAnnotation("Wired"));
print("Service ctor.hasAnnotation(Missing)=" + svcCtor.hasAnnotation("Missing"));

Annotation? wired = svcCtor.getAnnotation("Wired");
if (wired != null) {
    print("wired.scope=" + wired.getString("scope"));
}

print("Test passed");
