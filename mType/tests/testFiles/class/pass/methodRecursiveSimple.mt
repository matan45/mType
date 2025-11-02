// Test: Simple recursive method calls
// Expected: Pass - demonstrates recursion in methods

class MathUtils {
    // Recursive factorial
    public int factorial(int n) {
        print("factorial(" + n + ")");
        if (n <= 1) {
            return 1;
        }
        return n * this.factorial(n - 1);
    }

    // Recursive fibonacci
    public int fibonacci(int n) {
        print("fibonacci(" + n + ")");
        if (n <= 1) {
            return n;
        }
        return this.fibonacci(n - 1) + this.fibonacci(n - 2);
    }

    // Recursive countdown
    public void countdown(int n) {
        if (n <= 0) {
            print("Done!");
            return;
        }
        print("Count: " + n);
        this.countdown(n - 1);
    }
}

MathUtils math = new MathUtils();

print("Test 1: Factorial of 5");
int fact = math.factorial(5);
print("Result: " + fact);

print("\nTest 2: Fibonacci of 5");
int fib = math.fibonacci(5);
print("Result: " + fib);

print("\nTest 3: Countdown from 3");
math.countdown(3);
