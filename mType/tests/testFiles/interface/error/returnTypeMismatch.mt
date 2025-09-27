// Test interface return type mismatch error
interface Function {
    function apply(int x) : int;
}

class NotAFunction {
    int value;
    constructor(int v) { value = v; }
    function getValue() : int { return value; }
}

class Factory {
    function createBadFunction() : Function {
        return new NotAFunction(42);  // Returns object that doesn't implement Function
    }
}

print("Testing return type mismatch");
Factory factory = new Factory();
Function result = factory.createBadFunction();  // Error: Object type mismatch
print("Function created successfully");