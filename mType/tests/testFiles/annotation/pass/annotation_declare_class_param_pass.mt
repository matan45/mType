// MYT-108: single (non-array) Class-typed annotation parameter with named-arg
// usage and reflection back-readout via getClassName.
import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Method.mt";
import * from "../../lib/reflect/Annotation.mt";

class Worker { }

annotation Handler {
    Class type;
}

class Service {
    @Handler(type = Worker)
    public function ping(): void {
        print("ping");
    }
}

Class c = Class::forName("Service");
Method m = c.getDeclaredMethod("ping", 0);
Annotation? a = m.getAnnotation("Handler");
if (a != null) {
    print(a.getClassName("type"));
}

// Expected output:
// Worker
