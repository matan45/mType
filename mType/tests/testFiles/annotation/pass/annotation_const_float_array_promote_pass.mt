// MYT-376: a folded int element promotes the whole array to float[] (matching
// the existing int[] -> float[] coercion), mixing a constant expression with a
// named float constant.
import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Annotation.mt";

annotation Scales {
    float[] factors;
}

class Config {
    public static final float HALF = 0.5;
}

@Scales(factors = [2 * 10, Config::HALF])
class Target { }

Class c = Class::forName("Target");
Annotation? a = c.getAnnotation("Scales");
if (a != null) {
    float[] f = a.getFloatArray("factors");
    print("f0=" + (f[0] == 20.0));
    print("f1=" + (f[1] == 0.5));
}
