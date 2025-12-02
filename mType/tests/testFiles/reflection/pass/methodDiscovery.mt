// Test method discovery via reflection

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Method.mt";

class Calculator {
    private int value;

    public constructor() {
        this.value = 0;
    }

    public function add(int x): int {
        this.value = this.value + x;
        return this.value;
    }

    public function subtract(int x): int {
        this.value = this.value - x;
        return this.value;
    }

    public function getValue(): int {
        return this.value;
    }

    private function reset(): void {
        this.value = 0;
    }

    public static function create(): Calculator {
        return new Calculator();
    }
}

Class calcClass = Class::forName("Calculator");

// Get all declared methods
Method[] methods = calcClass.getDeclaredMethods();
print("Number of declared methods: " + methods.length);

// Print each method's name
for (Method m : methods) {
    print("Method: " + m.getName() + " static=" + m.isStatic());
}

// Get specific method by name
Method addMethod = calcClass.getDeclaredMethod("add", 1);
print("Found method 'add': " + addMethod.getName());

Method createMethod = calcClass.getDeclaredMethod("create", 0);
print("Found static method 'create': " + createMethod.getName());
print("create is static: " + createMethod.isStatic());

print("Test passed");
