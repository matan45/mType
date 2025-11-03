// Test: Basic method overloading with different parameter types and counts
// Expected: Pass - demonstrates method overloading resolution

class Calculator {
    // Overload 1: Single int parameter
    public function add(int a): int {
        print("add(int): " + a);
        return a;
    }

    // Overload 2: Two int parameters
    public function add2(int a, int b): int {
        print("add(int, int): " + a + " + " + b);
        return a + b;
    }

    // Overload 3: Three int parameters
    public function add3(int a, int b, int c): int {
        print("add(int, int, int): " + a + " + " + b + " + " + c);
        return a + b + c;
    }

    // Overload 4: String concatenation
    public function add4(string a, string b): string {
        print("add(string, string): " + a + " + " + b);
        return a + b;
    }

    // Overload 5: Mixed types
    public function add5(int a, string b): string {
        print("add(int, string): " + a + " + " + b);
        return a + b;
    }
}

Calculator calc = new Calculator();

print("Test 1: Single int");
int r1 = calc.add(5);
print("Result: " + r1);

print("\nTest 2: Two ints");
int r2 = calc.add2(10, 20);
print("Result: " + r2);

print("\nTest 3: Three ints");
int r3 = calc.add3(5, 10, 15);
print("Result: " + r3);

print("\nTest 4: Two strings");
string r4 = calc.add4("Hello", "World");
print("Result: " + r4);

print("\nTest 5: Int and string");
string r5 = calc.add5(42, "Answer");
print("Result: " + r5);
