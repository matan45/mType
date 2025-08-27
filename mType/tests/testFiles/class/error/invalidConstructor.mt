class TestClass {
    int value;
    
    // Only has constructor with parameter
    constructor(int v) {
        value = v;
    }
}

// Try to call constructor with wrong number of arguments
TestClass obj = new TestClass(); // Error: No matching constructor
print(obj);