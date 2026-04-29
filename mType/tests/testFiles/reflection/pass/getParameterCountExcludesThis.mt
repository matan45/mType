// MYT-214: Method.getParameterCount() returns the count of source-declared
// parameters and does NOT count the implicit `this` receiver — matching
// Java/.NET/Kotlin reflection.

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
}

Class cls = Class::forName("Processor");
Method[] methods = cls.getDeclaredMethods();

int paramSum = 0;
for (int i = 0; i < methods.length; i = i + 1) {
    Method m = methods[i];
    if (m.getName() == "process") {
        paramSum = paramSum + m.getParameterCount();
    }
}

print("sum of getParameterCount() across 3 process overloads: " + paramSum);
