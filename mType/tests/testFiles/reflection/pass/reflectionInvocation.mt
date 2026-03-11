// Reflection invocation test
// Tests __reflect_invokeMethod, __reflect_invokeStaticMethod, and __reflect_newInstanceWithArgs

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Field.mt";
import * from "../../lib/reflect/Method.mt";
import * from "../../lib/reflect/Constructor.mt";

// Test class with instance methods, static methods, and constructors with args
class Calculator {
    private int value;

    public constructor(int initial) {
        this.value = initial;
    }

    public function getValue(): int {
        return this.value;
    }

    public function add(int x): int {
        this.value = this.value + x;
        return this.value;
    }

    public function multiply(int x): int {
        this.value = this.value * x;
        return this.value;
    }

    public static function staticAdd(int a, int b): int {
        return a + b;
    }

    private function secretReset(): int {
        this.value = 0;
        return this.value;
    }
}

// === Test 1: Constructor invocation with args ===
print("=== Constructor Invocation ===");
Class calcClass = Class::forName("Calculator");
Constructor ctor = calcClass.getConstructor(1);
int[] ctorArgs = new int[1];
ctorArgs[0] = 42;
Calculator calc = __reflect_newInstanceWithArgs(ctor.getClassHandle(), ctor.getNativeHandle(), ctorArgs, ctor.getAccessible());
print("Created via reflection, value: " + calc.getValue());

// === Test 2: Instance method invocation ===
print("=== Instance Method Invocation ===");
Method addMethod = calcClass.getMethod("add", 1);
int[] addArgs = new int[1];
addArgs[0] = 8;
int result = __reflect_invokeMethod(calc, addMethod.getNativeHandle(), addArgs, addMethod.getAccessible());
print("After add(8): " + result);
print("getValue: " + calc.getValue());

// === Test 3: Another instance method ===
Method mulMethod = calcClass.getMethod("multiply", 1);
int[] mulArgs = new int[1];
mulArgs[0] = 3;
result = __reflect_invokeMethod(calc, mulMethod.getNativeHandle(), mulArgs, mulMethod.getAccessible());
print("After multiply(3): " + result);

// === Test 4: Static method invocation ===
print("=== Static Method Invocation ===");
Method staticAdd = calcClass.getMethod("staticAdd", 2);
int[] staticArgs = new int[2];
staticArgs[0] = 10;
staticArgs[1] = 20;
result = __reflect_invokeStaticMethod(staticAdd.getClassHandle(), staticAdd.getNativeHandle(), staticArgs, staticAdd.getAccessible());
print("staticAdd(10, 20): " + result);

// === Test 5: Private method with accessible=true ===
print("=== Private Method Access ===");
Method secretMethod = calcClass.getDeclaredMethod("secretReset", 0);
secretMethod.setAccessible(true);
int[] emptyArgs = new int[0];
result = __reflect_invokeMethod(calc, secretMethod.getNativeHandle(), emptyArgs, secretMethod.getAccessible());
print("After secretReset: " + result);
print("getValue after reset: " + calc.getValue());

// === Test 6: Create another instance via reflection ===
print("=== Second Constructor Invocation ===");
int[] ctorArgs2 = new int[1];
ctorArgs2[0] = 100;
Calculator calc2 = __reflect_newInstanceWithArgs(ctor.getClassHandle(), ctor.getNativeHandle(), ctorArgs2, ctor.getAccessible());
print("Second instance value: " + calc2.getValue());

print("All reflection invocation tests passed");
