// Test interface parameter type mismatch error
interface Function {
    function apply(int x) : int;
}

class NotAFunction {
    int value;
    constructor(int v) { value = v; }
    function getValue() : int { return value; }
}

class Processor {
    function processFunction(Function f, int value) : int {
        return f.apply(value);  // Error: NotAFunction doesn't have apply method
    }
}

print("Testing parameter type mismatch");
Processor proc = new Processor();
NotAFunction badObj = new NotAFunction(10);
int result = proc.processFunction(badObj, 5);  // Should fail when apply is called
print("Result: " + result);