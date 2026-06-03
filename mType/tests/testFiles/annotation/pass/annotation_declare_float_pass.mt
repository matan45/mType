// MYT-108: float-typed annotation parameter with named-arg usage and reflection
// back-readout via getFloat. Value is compared (not printed) to avoid relying
// on float print formatting.
import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Method.mt";
import * from "../../lib/reflect/Annotation.mt";

annotation Weight {
    float value;
}

class Service {
    @Weight(value = 1.5)
    public function ping(): void {
        print("ping");
    }
}

Class c = Class::forName("Service");
Method m = c.getDeclaredMethod("ping", 0);
Annotation? a = m.getAnnotation("Weight");
if (a != null) {
    print(a.getFloat("value") == 1.5);
}

// Expected output:
// true
