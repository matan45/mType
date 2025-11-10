// Different return types in branches (should error)
interface Function {
    function apply(int x) : int;
}

// This should fail - branches return incompatible types
Function badLambda = x -> {
    if (x > 0) {
        return x;
    } else {
        return "negative";  // String instead of int
    }
};
