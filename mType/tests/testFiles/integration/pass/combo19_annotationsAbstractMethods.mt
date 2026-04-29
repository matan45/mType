// Combo 19: Annotations + Abstract Methods + Override + Reflection
// Tests: abstract methods carry @Override and @Trace; concrete subclass inherits/overrides; reflect on annotations

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Method.mt";
import * from "../../lib/reflect/Annotation.mt";

@Retention(RUNTIME)
@Target([METHOD])
annotation Trace {
    string tag = "default";
}

abstract class Worker {
    @Trace(tag = "compute")
    public abstract function compute(int x): int;

    @Trace(tag = "describe")
    public abstract function describe(): string;
}

class Doubler extends Worker {
    @Override
    @Trace(tag = "compute-doubler")
    public function compute(int x): int {
        return x * 2;
    }

    @Override
    @Trace(tag = "describe-doubler")
    public function describe(): string {
        return "Doubler";
    }
}

function reflectAndPrint(string className): void {
    Class cls = Class::forName(className);
    print("Class: " + cls.getName());
    Method[] methods = cls.getDeclaredMethods();
    print("Method count: " + methods.length);
    for (int i = 0; i < methods.length; i++) {
        Method m = methods[i];
        if (m.hasAnnotation("Trace")) {
            Annotation? trace = m.getAnnotation("Trace");
            if (trace != null) {
                print("  " + m.getName() + " @Trace(tag=" + trace.getString("tag") + ")");
            }
        }
    }
}

function main(): void {
    print("=== Combo 19: Annotations + Abstract Methods ===");

    print("--- Direct calls ---");
    Doubler d = new Doubler();
    print("compute(5) = " + d.compute(5));
    print("describe() = " + d.describe());

    print("--- Reflect Worker ---");
    reflectAndPrint("Worker");

    print("--- Reflect Doubler ---");
    reflectAndPrint("Doubler");

    print("=== Combo 19 Complete ===");
}

main();
