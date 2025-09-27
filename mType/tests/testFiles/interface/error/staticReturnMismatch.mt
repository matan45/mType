// Test interface return type mismatch with static methods
interface BinaryFunction {
    function apply(int a, int b) : int;
}

class NotABinaryFunction {
    string operation;
    constructor(string op) { operation = op; }
    function getOperation() : string { return operation; }
}

class StaticFactory {
    static function createBadBinaryFunction() : BinaryFunction {
        return new NotABinaryFunction("invalid");  // Returns object that doesn't implement BinaryFunction
    }
}

print("Testing static method return mismatch");
BinaryFunction result = StaticFactory::createBadBinaryFunction();  // Error: Object type mismatch
print("Binary function created successfully");