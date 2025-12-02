// Test constructor discovery via reflection

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Constructor.mt";

class MultiConstructor {
    private int x;
    private int y;

    public constructor() {
        this.x = 0;
        this.y = 0;
    }

    public constructor(int x) {
        this.x = x;
        this.y = 0;
    }

    public constructor(int x, int y) {
        this.x = x;
        this.y = y;
    }
}

Class testClass = Class::forName("MultiConstructor");

// Get all constructors
Constructor[] constructors = testClass.getDeclaredConstructors();
print("Number of constructors: " + constructors.length);

// Print each constructor's parameter count
for (Constructor c : constructors) {
    print("Constructor param count: " + c.getParameterCount());
}

// Get specific constructor by parameter count
Constructor defaultCtor = testClass.getDeclaredConstructor(0);
print("Default constructor params: " + defaultCtor.getParameterCount());

Constructor twoArgCtor = testClass.getDeclaredConstructor(2);
print("Two-arg constructor params: " + twoArgCtor.getParameterCount());

print("Test passed");
