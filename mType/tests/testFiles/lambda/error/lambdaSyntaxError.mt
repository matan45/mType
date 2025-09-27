// Lambda syntax error test
interface Function {
    function apply(x: int) : int;
}

// This should fail - invalid lambda syntax
Function badLambda = x => x * 2;  // Should be -> not =>