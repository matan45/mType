// Lambda parameter type mismatch test
interface Function {
    function apply(int x) : int;
}

// This should fail - lambda expects string but interface expects int
Function badLambda = (string s) -> s.length();