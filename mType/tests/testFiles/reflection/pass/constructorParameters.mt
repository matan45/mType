// Test constructor parameter introspection

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Constructor.mt";

class Person {
    private string name;
    private int age;
    private bool active;

    public constructor(string name, int age, bool active) {
        this.name = name;
        this.age = age;
        this.active = active;
    }
}

Class personClass = Class::forName("Person");

// Get the constructor
Constructor ctor = personClass.getDeclaredConstructor(3);
print("Constructor param count: " + ctor.getParameterCount());

// Get parameter types
string[] paramTypes = ctor.getParameterTypes();
print("Number of param types: " + paramTypes.length);

for (int i = 0; i < paramTypes.length; i = i + 1) {
    print("Param " + i + " type: " + paramTypes[i]);
}

// Check constructor modifiers
print("Constructor is public: " + ctor.isPublic());
print("Constructor is private: " + ctor.isPrivate());

print("Test passed");
