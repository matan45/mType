// Test: Method with missing return statement
// Expected: Compilation error - non-void method must return on all paths

class Calculator {
    // This method declares int return type but has no return statement
    public function add(int a, int b): int {
        int sum = a + b;
        // Missing return statement - should cause compilation error
    }
}

Calculator c = new Calculator();
print(c.add(5, 3));
