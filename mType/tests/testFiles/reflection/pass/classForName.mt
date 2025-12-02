// Test Class.forName functionality

import * from "../../lib/reflect/Class.mt";

class SimpleClass {
    public int value;

    public constructor() {
        this.value = 42;
    }
}

// Get Class object using forName
Class simpleClass = Class::forName("SimpleClass");

// Verify class name
print("Class name: " + simpleClass.getName());
print("Simple name: " + simpleClass.getSimpleName());

// Verify class was found
print("Handle valid: " + (simpleClass.getNativeHandle() > 0));

print("Test passed");
