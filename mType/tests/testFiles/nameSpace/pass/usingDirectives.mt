namespace math {
int value = 42;

function multiply(int a, int b): int {
    return a * b;
}

function square(int x): int {
    return x * x;
}

namespace advanced {
    function factorial(int n): int {
        if (n <= 1) {
            return 1;
        }
        int result = 1;
        for (int i = 2; i <= n; i++) {
            result = result * i;
        }
        return result;
    }
    
    function fibonacci(int n): int {
        if (n <= 1) {
            return n;
        }
        return fibonacci(n - 1) + fibonacci(n - 2);
    }
}
}

// Test qualified calls first
int product1 = math::multiply(8, 9);
print(product1);

int square1 = math::square(7);
print(square1);

int fact1 = math::advanced::factorial(4);
print(fact1);

// Test using directives
using namespace math;
using namespace math::advanced;

int square2 = square(7);
print(square2);
int fact2 = factorial(4);
print(fact2);