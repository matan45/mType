// Test: Constructor with void return type
class TestClass3 {
    bool flag;
    
    constructor(): void {  // Should error
        flag = true;
    }
}

// This code should never be reached due to parse error
print("This should not print - Test 3");