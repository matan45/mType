// Test lambda return type mismatch error
interface Calculator {
    function calculate(int x) : int;
}

// Lambda returns string but interface expects int - should fail type checking
Calculator badCalc = x -> "Result: " + x;
