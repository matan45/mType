namespace algorithms {
function fibonacci(int n): int {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

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

function isEven(int n): bool {
    return (n % 2) == 0;
}

function countToN(int n): int {
    int sum = 0;
    for (int i = 1; i <= n; i++) {
        sum = sum + i;
    }
    return sum;
}

namespace sorting {
    function simpleSort(int a, int b, int c): int {
        if (a > b && a > c) {
            return a;
        }
        if (b > c) {
            return b;
        }
        return c;
    }
}

namespace search {
    function linearSearch(int target): bool {
        for (int i = 0; i < 10; i++) {
            if (i == target) {
                return true;
            }
        }
        return false;
    }
    
    function findMax(int a, int b): int {
        if (a > b) {
            return a;
        }
        return b;
    }
}
}

// Test complex algorithms with control flow
int fib = algorithms::fibonacci(6);
print(fib);

int fact = algorithms::factorial(5);
print(fact);

bool even = algorithms::isEven(8);
print(even);

int sum = algorithms::countToN(10);
print(sum);

int maxVal = algorithms::sorting::simpleSort(15, 23, 19);
print(maxVal);

bool found = algorithms::search::linearSearch(7);
print(found);

int maximum = algorithms::search::findMax(42, 37);
print(maximum);