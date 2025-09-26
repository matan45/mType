// Complex block lambda with control flow
interface Processor {
    function process(n: int) : int;
}

print("=== Complex Block Lambda Test ===");

Processor factorial = n -> {
    if (n <= 1) {
        return 1;
    }
    int result = 1;
    int i = 2;
    while (i <= n) {
        result = result * i;
        i = i + 1;
    }
    return result;
};

print("Factorial of 5: " + factorial.process(5));
print("Factorial of 3: " + factorial.process(3));