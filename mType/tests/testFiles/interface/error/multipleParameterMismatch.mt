// Test multiple interface parameters with type mismatch
interface Function {
    function apply(int x) : int;
}

interface Predicate {
    function test(int x) : bool;
}

class NotAFunction {
    int value;
    constructor(int v) { value = v; }
    function getValue() : int { return value; }
}

class NotAPredicate {
    string name;
    constructor(string n) { name = n; }
    function getName() : string { return name; }
}

class ComplexProcessor {
    function processConditional(Function mapper, Predicate filter, int value) : int {
        if (filter.test(value)) {  // Error: NotAPredicate doesn't have test method
            return mapper.apply(value);
        }
        return -1;
    }
}

print("Testing multiple parameter type mismatch");
ComplexProcessor proc = new ComplexProcessor();
NotAFunction badMapper = new NotAFunction(2);
NotAPredicate badFilter = new NotAPredicate("bad");
int result = proc.processConditional(badMapper, badFilter, 5);  // Should fail
print("Result: " + result);