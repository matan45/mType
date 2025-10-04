class TestClass {
    int value;
    constructor(int val) {
        value = val;
    }
    
    public function getValue(): int {
        return value;
    }
}

TestClass obj = new TestClass(42);
int result = obj.getValue();
print(result);
print("Test passed");