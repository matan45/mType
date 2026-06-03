// MYT-108: bool-typed annotation parameter with named-arg usage and reflection
// back-readout via getBool.
import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Method.mt";
import * from "../../lib/reflect/Annotation.mt";

annotation Enabled {
    bool value;
}

class Service {
    @Enabled(value = true)
    public function ping(): void {
        print("ping");
    }
}

Class c = Class::forName("Service");
Method m = c.getDeclaredMethod("ping", 0);
Annotation? a = m.getAnnotation("Enabled");
if (a != null) {
    print(a.getBool("value"));
}

// Expected output:
// true
