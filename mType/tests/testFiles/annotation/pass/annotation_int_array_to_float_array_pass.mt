// MYT-375: an int[] literal is accepted where float[] is declared and each
// element is widened to a double (validator isAssignable + coerceValue).
import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Annotation.mt";

@Retention(RUNTIME)
annotation Ratios {
    float[] values;
}

@Ratios(values = [1, 2, 3])
class Target { }

Class c = Class::forName("Target");
Annotation? r = c.getAnnotation("Ratios");
if (r != null) {
    float[] v = r.getFloatArray("values");
    print("ok=" + (v[0] == 1.0) + "," + (v[1] == 2.0) + "," + (v[2] == 3.0));
}

// Expected output:
// ok=true,true,true
