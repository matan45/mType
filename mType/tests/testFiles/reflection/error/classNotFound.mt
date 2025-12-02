// Test: Class.forName with non-existent class should throw error

import * from "../../lib/reflect/Class.mt";

// This should throw a runtime error
Class nonExistent = Class::forName("NonExistentClass");

print("Should not reach here");
