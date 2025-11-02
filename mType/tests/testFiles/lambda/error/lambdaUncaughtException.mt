// Lambda throwing uncaught exception (should error)
interface Function {
    function apply(int x) : int;
}

// This should fail - lambda throws exception that's not caught
Function thrower = x -> {
    if (x < 0) {
        throw "Negative value error";
    }
    return x * 2;
};
