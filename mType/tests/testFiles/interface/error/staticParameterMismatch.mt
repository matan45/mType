// Test interface parameter type mismatch with static methods
interface Predicate {
    function test(int x) : bool;
}

class NotAPredicate {
    string name;
    constructor(string n) { name = n; }
    function getName() : string { return name; }
}

class StaticProcessor {
    static function filterValue(Predicate pred, int value) : bool {
        return pred.test(value);  // Error: NotAPredicate doesn't have test method
    }
}

print("Testing static method parameter mismatch");
NotAPredicate badPred = new NotAPredicate("invalid");
bool result = StaticProcessor::filterValue(badPred, 10);  // Should fail when test is called
print("Filter result: " + result);