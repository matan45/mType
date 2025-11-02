// Code after return in lambda (should error)
interface Function {
    function apply(int x) : int;
}

// This should fail - code after return is unreachable
Function badLambda = x -> {
    return x * 2;
    int unreachable = x + 1;  // Unreachable code
};
