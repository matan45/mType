// MYT-375: user-defined annotation arrays, Class[] reflection, nullable
// reference parameter default, and SOURCE retention stripping.

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Annotation.mt";

class A { }
class B { }

@Retention(RUNTIME)
annotation Uses {
    Class[] types;
    int[] weights;
    float[] ratios = [1, 2.5];
    bool[] flags;
    string[] names;
    string? optional = null;
}

@Retention(SOURCE)
annotation SourceOnly {
    string[] names;
}

@Uses(types = [A, B], weights = [3, 5], flags = [true, false], names = ["left", "right"])
@SourceOnly(names = ["hidden"])
class Target { }

Class c = Class::forName("Target");
Annotation? uses = c.getAnnotation("Uses");

if (uses != null) {
    string[] types = uses.getClassNames("types");
    int[] weights = uses.getIntArray("weights");
    float[] ratios = uses.getFloatArray("ratios");
    bool[] flags = uses.getBoolArray("flags");
    string[] names = uses.getStringArray("names");

    print("types=" + types[0] + "," + types[1]);
    print("weights=" + weights[0] + "," + weights[1]);
    print("ratiosOk=" + (ratios[0] == 1.0) + "," + (ratios[1] == 2.5));
    print("flags=" + flags[0] + "," + flags[1]);
    print("names=" + names[0] + "," + names[1]);
    print("optionalNull=" + uses.isNull("optional"));
}

print("hasSourceOnly=" + c.hasAnnotation("SourceOnly"));
