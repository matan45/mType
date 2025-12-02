// Test method return type introspection

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Method.mt";

class ReturnTypes {
    public function returnsInt(): int {
        return 42;
    }

    public function returnsFloat(): float {
        return 3.14;
    }

    public function returnsBool(): bool {
        return true;
    }

    public function returnsString(): string {
        return "hello";
    }

    public function returnsVoid(): void {
        // nothing
    }
}

Class testClass = Class::forName("ReturnTypes");

Method intMethod = testClass.getDeclaredMethod("returnsInt", 0);
print("returnsInt return type: " + intMethod.getReturnType());

Method floatMethod = testClass.getDeclaredMethod("returnsFloat", 0);
print("returnsFloat return type: " + floatMethod.getReturnType());

Method boolMethod = testClass.getDeclaredMethod("returnsBool", 0);
print("returnsBool return type: " + boolMethod.getReturnType());

Method stringMethod = testClass.getDeclaredMethod("returnsString", 0);
print("returnsString return type: " + stringMethod.getReturnType());

Method voidMethod = testClass.getDeclaredMethod("returnsVoid", 0);
print("returnsVoid return type: " + voidMethod.getReturnType());

print("Test passed");
