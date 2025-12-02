// Test: getDeclaredMethod with non-existent method should throw error

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Method.mt";

class TestClass {
    public function existingMethod(): void {}
}

Class testClass = Class::forName("TestClass");

// This should throw a runtime error
Method nonExistent = testClass.getDeclaredMethod("nonExistentMethod", 0);

print("Should not reach here");
