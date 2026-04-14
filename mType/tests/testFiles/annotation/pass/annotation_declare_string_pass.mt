// MYT-108 §7a case 5: string-typed annotation parameter via positional shorthand
// (single-param annotations accept a bare literal as positional binding).
import * from "lib/reflect/Class.mt";
import * from "lib/reflect/Method.mt";
import * from "lib/reflect/Annotation.mt";

annotation DisplayName {
    string value;
}

class Calc {
    @DisplayName("adds two positive integers")
    public function add(): void {
        print("ok");
    }
}

Class c = Class::forName("Calc");
Method m = c.getDeclaredMethod("add");
Annotation d = m.getAnnotation("DisplayName");
print(d.getString("value"));
