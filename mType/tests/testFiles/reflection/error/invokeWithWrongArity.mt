// Reflectively invoking a method with the wrong number of arguments must
// throw. No existing reflection error test covers this. Pins the arity
// mismatch contract through VirtualMachine::invokeMethod.

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Method.mt";
import * from "../../lib/reflect/Constructor.mt";

class Adder {
    public constructor() { }
    public function add(int a, int b): int {
        return a + b;
    }
}

Class cls = Class::forName("Adder");
Constructor ctor = cls.getDeclaredConstructor(0);
int[] noArgs = new int[0];
Adder inst = __reflect_newInstanceWithArgs(ctor.getClassHandle(), ctor.getNativeHandle(), noArgs, ctor.isAccessible());

Method add = cls.getDeclaredMethod("add", 2);
int[] onlyOne = new int[1];
onlyOne[0] = 5;
int result = __reflect_invokeMethod(inst, add.getNativeHandle(), onlyOne, add.isAccessible());
print("FAIL: invoke with 1 arg of 2-arg method returned " + result);
