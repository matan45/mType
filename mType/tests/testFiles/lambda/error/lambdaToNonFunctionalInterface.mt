// Lambda assigned to non-functional interface test
interface MultiMethodInterface {
    function method1() : void;
    function method2() : int;
}

// This should fail - interface has multiple methods (not functional)
MultiMethodInterface badLambda = () -> print("Hello");