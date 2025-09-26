// Lambda parameter type mismatch test
interface Function {
    function apply(x: int) : int;
}

// This should fail - lambda expects string but interface expects int
Function badLambda = (s: string) -> s.length();