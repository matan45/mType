// MYT-108 §7a case 2: int-typed annotation parameter with named-arg usage
// and reflection back-readout via getInt.
import * from "lib/reflect/Class.mt";
import * from "lib/reflect/Method.mt";
import * from "lib/reflect/Annotation.mt";

annotation Timeout {
    int ms;
}

class Service {
    @Timeout(ms = 5000)
    public function ping(): void {
        print("ping");
    }
}

Class c = Class::forName("Service");
Method m = c.getDeclaredMethod("ping");
Annotation t = m.getAnnotation("Timeout");
print(t.getInt("ms"));
