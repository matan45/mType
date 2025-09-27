// Lambda return type mismatch test
interface Function {
    function apply(x: int) : int;
}

// This should fail - lambda returns string but interface expects int
Function badLambda = x -> "result: " + x;