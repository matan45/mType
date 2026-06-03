// MYT-376: a const-folded annotation argument on a METHOD (not a class) — the
// resolver recurses into methods, so folding must apply there too.
import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Method.mt";
import * from "../../lib/reflect/Annotation.mt";

annotation Timeout {
    int ms;
}

class Service {
    @Timeout(ms = 30 * 2)
    public function ping(): void {
        print("ping");
    }
}

Class c = Class::forName("Service");
Method m = c.getDeclaredMethod("ping", 0);
Annotation? a = m.getAnnotation("Timeout");
if (a != null) {
    print(a.getInt("ms"));
}

// Expected output:
// 60
