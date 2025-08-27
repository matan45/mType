class TestClass {
    final int finalValue;
    
    constructor() {
        finalValue = 42;
    }
    
    // Instance method return types cannot be final
    function getFinalValue(): final int {
        return finalValue;
    }
    
    // Static method return types cannot be final
    static function getConstant(): final float {
        return 3.14159;
    }
    
    // Mixed scenario - valid method with invalid return type
    function computeSum(int a, int b): final int {
        return a + b;
    }
}

// This code should never be reached due to parse error
print("This should not print");