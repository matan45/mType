// Function parameters cannot be final
function globalFunction(final int param): void {
    print(param);
}

class TestClass {
    // Method parameters cannot be final
    function method(final string param): void {
        print(param);
    }
    
    // Constructor parameters cannot be final
    constructor(final int param) {
        print(param);
    }
}

// This code should never be reached due to parse error
print("This should not print");