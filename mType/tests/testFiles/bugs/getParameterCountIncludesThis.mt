// MYT-214: Method.getParameterCount() includes `this` for instance methods.
//
// EXPECTED (matching Java/.NET/Kotlin reflection):
//   process(int x)         getParameterCount() == 1
//   process(string s)      getParameterCount() == 1
//   process(int x, int y)  getParameterCount() == 2
//   sum                    == 4
//
// ACTUAL (broken):
//   process(int x)         getParameterCount() == 2  (includes this)
//   process(string s)      getParameterCount() == 2
//   process(int x, int y)  getParameterCount() == 3
//   sum                    == 7

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
print("expected (excluding this): 4");
print("actual (including this):   7");
