// Basic block lambda test
interface Function {
    function apply(x: int) : int;
}

print("=== Basic Block Lambda Test ===");

Function calculator = x -> {
    int temp = x * x;
    temp = temp + x;
    return temp - 1;
};

print("Calculator(3) = " + calculator.apply(3));
print("Calculator(5) = " + calculator.apply(5));