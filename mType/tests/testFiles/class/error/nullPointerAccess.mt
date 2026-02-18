class TestClass {
    int value;
    
    function getValue(): int {
        return value;
    }
}

TestClass? obj = null;
print(obj); // Should print null

// This should throw an error
int val = obj.getValue(); // Error: Null pointer exception
print(val);