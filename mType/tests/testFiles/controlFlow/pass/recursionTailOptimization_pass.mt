// Test tail recursion pattern
// These are tail-recursive functions where the recursive call is the last operation

// Tail-recursive factorial with accumulator
function factorialTail(int n, int acc): int {
    if (n <= 1) {
        return acc;
    }
    return factorialTail(n - 1, n * acc);
}

function factorial(int n): int {
    return factorialTail(n, 1);
}

// Tail-recursive sum of numbers
function sumToNTail(int n, int acc): int {
    if (n <= 0) {
        return acc;
    }
    return sumToNTail(n - 1, acc + n);
}

function sumToN(int n): int {
    return sumToNTail(n, 0);
}

// Tail-recursive countdown
function countdownTail(int n, int acc): int {
    if (n <= 0) {
        return acc;
    }
    return countdownTail(n - 1, acc + 1);
}

function countdown(int start): int {
    return countdownTail(start, 0);
}

// Tail-recursive power function
function powerTail(int base, int exp, int acc): int {
    if (exp <= 0) {
        return acc;
    }
    return powerTail(base, exp - 1, acc * base);
}

function power(int base, int exp): int {
    return powerTail(base, exp, 1);
}

// Execute tests
print("Testing tail recursion patterns:");

print("factorial(5) = " + factorial(5));           // 120
print("factorial(6) = " + factorial(6));           // 720
print("factorial(7) = " + factorial(7));           // 5040

print("sumToN(10) = " + sumToN(10));               // 55
print("sumToN(100) = " + sumToN(100));             // 5050

print("countdown(5) = " + countdown(5));           // 5
print("countdown(10) = " + countdown(10));         // 10

print("power(2, 10) = " + power(2, 10));           // 1024
print("power(3, 5) = " + power(3, 5));             // 243

print("Tail recursion tests completed successfully");
