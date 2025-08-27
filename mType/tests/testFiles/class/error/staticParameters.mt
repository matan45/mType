// Function parameters cannot be static
function globalFunction(static int param): void {
    print(param);
}

class TestClass {
    // Method parameters cannot be static
    function method(static string param): void {
        print(param);
    }
    
    // Constructor parameters cannot be static
    constructor(static int param) {
        print(param);
    }
}

// This code should never be reached due to parse error
print("This should not print");