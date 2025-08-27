// Constructors cannot be final
class TestClass {
    int value;
    final int finalField;
    
    // This should cause an error - constructors cannot be final
    final constructor() {
        value = 10;
        finalField = 20;
    }
}

// This code should never be reached due to parse error
print("This should not print");