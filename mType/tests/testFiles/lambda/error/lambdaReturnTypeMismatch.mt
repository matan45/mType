// Lambda return type mismatch test
interface Function {
    function apply(int x) : int;
}

// This should fail - lambda returns string but interface expects int
Function badLambda = x -> "result: " + x;