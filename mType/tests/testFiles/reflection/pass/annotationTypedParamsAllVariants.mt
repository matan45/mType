// Drive every typed Annotation getter through a reflectively-obtained handle.
// annotations_demo_pass.mt only exercises getString + getBool. This pins
// getInt, getFloat (with int-widening), getClassName, isNull.

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Method.mt";
import * from "../../lib/reflect/Annotation.mt";

annotation Spec {
    int port;
    float ratio;
    string label = "default";
    bool enabled = true;
    Class handler;
    string? optional = null;
}

class Owner { }

@Spec(port = 8080, ratio = 1.5, label = "x", enabled = false, handler = Owner)
class Configured { }

Class cls = Class::forName("Configured");
print("hasAnnotation(Spec)=" + cls.hasAnnotation("Spec"));

Annotation? a = cls.getAnnotation("Spec");
if (a != null) {
    print("port=" + a.getInt("port"));
    print("ratio=" + a.getFloat("ratio"));
    print("label=" + a.getString("label"));
    print("enabled=" + a.getBool("enabled"));
    print("handler=" + a.getClassName("handler"));
    print("optional isNull=" + a.isNull("optional"));
    print("label isNull=" + a.isNull("label"));
}

print("Test passed");
