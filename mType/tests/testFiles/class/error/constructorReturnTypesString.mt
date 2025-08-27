// Test: Constructor with string return type
class TestClass2 {
    string name;
    
    constructor(string n): string {  // Should error
        name = n;
    }
}

// This code should never be reached due to parse error
print("This should not print - Test 2");