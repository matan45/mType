// Methods cannot be final
class TestClass {
    int value;
    final int finalField;
    
    constructor() {
        value = 10;
        finalField = 20;
    }
    
    // This should cause an error - methods cannot be final
    final function getValue(): int {
        return value;
    }
    
    // Final fields are allowed
    function getFinalField(): int {
        return finalField;
    }
}

// This code should never be reached due to parse error
print("This should not print");