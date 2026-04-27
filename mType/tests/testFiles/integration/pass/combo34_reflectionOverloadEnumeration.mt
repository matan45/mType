// Combo 34: Reflection enumeration of overloaded methods
// Tests: getDeclaredMethods finds all three "process" overloads with their parameter counts

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Method.mt";

class Processor {
    public function process(int x): string {
        return "int:" + x;
    }

    public function process(string s): string {
        return "str:" + s;
    }

    public function process(int x, int y): string {
        return "pair:" + x + "," + y;
    }

    public function name(): string {
        return "Processor";
    }
}

function main(): void {
    print("=== Combo 34: Reflection Overload Enumeration ===");

    Processor p = new Processor();
    print("--- Direct calls ---");
    print(p.process(7));
    print(p.process("hi"));
    print(p.process(3, 4));

    print("--- Reflection enumeration ---");
    Class cls = Class::forName("Processor");
    print("Class: " + cls.getName());

    Method[] methods = cls.getDeclaredMethods();
    int processCount = 0;
    int paramSum = 0;
    bool sawZero = false;
    bool sawOne = false;
    bool sawTwo = false;
    for (int i = 0; i < methods.length; i++) {
        Method m = methods[i];
        if (m.getName() == "process") {
            processCount = processCount + 1;
            int pc = m.getParameterCount();
            paramSum = paramSum + pc;
            if (pc == 0) { sawZero = true; }
            if (pc == 1) { sawOne = true; }
            if (pc == 2) { sawTwo = true; }
        }
    }
    print("process overload total=" + processCount);
    print("process param sum=" + paramSum);
    print("has zero-arg overload=" + sawZero);
    print("has one-arg overload=" + sawOne);
    print("has two-arg overload=" + sawTwo);

    print("=== Combo 34 Complete ===");
}

main();
