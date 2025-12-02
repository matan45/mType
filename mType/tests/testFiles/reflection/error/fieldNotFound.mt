// Test: getDeclaredField with non-existent field should throw error

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Field.mt";

class TestClass {
    public int existingField;
}

Class testClass = Class::forName("TestClass");

// This should throw a runtime error
Field nonExistent = testClass.getDeclaredField("nonExistentField");

print("Should not reach here");
