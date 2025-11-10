// Lambda parameter count mismatch test
interface BinaryFunction {
    function apply(int a, int b) : int;
}

// This should fail - lambda has 1 parameter but interface expects 2
BinaryFunction badLambda = x -> x * 2;