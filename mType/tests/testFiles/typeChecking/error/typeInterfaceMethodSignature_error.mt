// Test method signature mismatch error - wrong parameter type
interface Calculator {
    function add(int a, int b): int;
    function multiply(float a, float b): float;
}

class BadCalculator implements Calculator {
    // Error: parameter type mismatch - should be int, not string
    public function add(string a, int b): int {
        return 0;
    }

    // Error: return type mismatch - should be float, not int
    public function multiply(float a, float b): int {
        return 0;
    }
}

BadCalculator calc = new BadCalculator();
print("This should fail");
