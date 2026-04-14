// MYT-108 §7a case 10: mix of defaulted + required parameters; caller
// overrides a subset and the validator fills in the remaining defaults.
import * from "lib/reflect/Class.mt";
import * from "lib/reflect/Method.mt";
import * from "lib/reflect/Annotation.mt";

annotation Retry {
    int times = 3;
    int delay = 100;
}

class Net {
    @Retry(times = 5)
    public function call(): void {
        print("called");
    }
}

Class c = Class::forName("Net");
Method m = c.getDeclaredMethod("call");
Annotation r = m.getAnnotation("Retry");
print(r.getInt("times"));
print(r.getInt("delay"));
