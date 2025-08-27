class TestClass {
    int value;
    
    constructor() {
        value = 100;
    }
    
    // Instance method return types cannot be static
    function getValue(): static int {
        return value;
    }
    
    // Static method return types cannot be static
    static function getStaticValue(): static string {
        return "static";
    }
}

// This code should never be reached due to parse error
print("This should not print");