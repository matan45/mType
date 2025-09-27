// Wrong return type in interface implementation
interface Calculator {
    function add(int a, int b): int;
}

class BadCalculator implements Calculator {
    function add(int a, int b): float {  // Wrong return type - should be int
        return 5.0;
    }
}

print("This should not print");