// Constructors cannot be static
class TestClass {
    int value;
    static int staticValue = 50;
    
    // This should cause an error - constructors cannot be static
    static constructor() {
        value = 10;
    }
    
    // Static methods are allowed
    static function getStaticValue(): int {
        return staticValue;
    }
}

// This code should never be reached due to parse error
print("This should not print");