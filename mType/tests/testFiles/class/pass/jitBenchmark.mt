// JIT Benchmark: Heavy computation to measure JIT vs interpreter performance
// Run with: mType --jit-stats jitBenchmark.mt
// Compare:  mType --no-jit jitBenchmark.mt

// Pure integer arithmetic — best case for JIT
function fibonacci(int n): int {
    int a = 0;
    int b = 1;
    for (int i = 0; i < n; i = i + 1) {
        int temp = b;
        b = a + b;
        a = temp;
    }
    return a;
}

// Nested loop with mixed arithmetic
function matrixSum(int size): int {
    int sum = 0;
    for (int i = 0; i < size; i = i + 1) {
        for (int j = 0; j < size; j = j + 1) {
            sum = sum + (i * size + j);
        }
    }
    return sum;
}

// Function call heavy — tests JIT dispatch
function square(int x): int {
    return x * x;
}

function callHeavy(int n): int {
    int total = 0;
    for (int i = 0; i < n; i = i + 1) {
        total = total + square(i);
    }
    return total;
}

// Conditional branching in hot loop
function collatz(int n): int {
    int steps = 0;
    while (n != 1) {
        if (n % 2 == 0) {
            n = n / 2;
        } else {
            n = n * 3 + 1;
        }
        steps = steps + 1;
    }
    return steps;
}

function collatzBench(int limit): int {
    int maxSteps = 0;
    for (int i = 2; i < limit; i = i + 1) {
        int steps = collatz(i);
        if (steps > maxSteps) {
            maxSteps = steps;
        }
    }
    return maxSteps;
}

// === Run benchmarks ===

// Warm up JIT with initial calls
for (int i = 0; i < 150; i = i + 1) {
    fibonacci(10);
    square(i);
}

print("=== JIT Benchmark ===");

// Test 1: Fibonacci (loop-heavy integer arithmetic)
int fibResult = 0;
for (int i = 0; i < 10000; i = i + 1) {
    fibResult = fibonacci(50);
}
print("Fibonacci(50) x10000: " + fibResult);

// Test 2: Matrix sum (nested loops)
int matResult = matrixSum(500);
print("MatrixSum(500): " + matResult);

// Test 3: Function call heavy
int callResult = callHeavy(100000);
print("CallHeavy(100000): " + callResult);

// Test 4: Collatz conjecture (branching)
int collatzResult = collatzBench(5000);
print("Collatz max steps (2..5000): " + collatzResult);

print("=== Benchmark complete ===");
