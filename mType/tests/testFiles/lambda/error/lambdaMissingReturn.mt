// Lambda block without return for non-void (should error)
interface Function {
    function apply(int x) : int;
}

// This should fail - lambda expects int return but no return statement
Function badLambda = x -> {
    int temp = x * 2;
    // Missing return statement
};
