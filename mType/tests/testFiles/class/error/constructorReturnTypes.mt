// Test: Constructor with int return type  
class TestClass1 {
    int value;
    
    constructor(): int {  // Should error
        value = 42;
    }
}

// This code should never be reached due to parse error
print("This should not print - Test 1");
TestClass1 obj1 = new TestClass1();