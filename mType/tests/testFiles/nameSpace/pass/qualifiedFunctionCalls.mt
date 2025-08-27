namespace calculator {
function add(int a, int b): int {
    return a + b;
}

function subtract(int a, int b): int {
    return a - b;
}

function multiply(int a, int b): int {
    return a * b;
}

namespace scientific {
    function power(int base, int exp): int {
        int result = 1;
        for (int i = 0; i < exp; i++) {
            result = result * base;
        }
        return result;
    }
    
    function factorial(int n): int {
        if (n <= 1) {
            return 1;
        }
        return n * factorial(n - 1);
    }
}
}

// Test various qualified function calls
int sum = calculator::add(10, 15);
print(sum);

int difference = calculator::subtract(20, 8);
print(difference);

int product = calculator::multiply(6, 7);
print(product);

int powered = calculator::scientific::power(2, 4);
print(powered);

int fact = calculator::scientific::factorial(5);
print(fact);