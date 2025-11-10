// Test call stack depth tracking with deep recursion

// Simple recursive function with reasonable depth
function countDown(int n): int {
    if (n <= 0) {
        return 0;
    }
    return 1 + countDown(n - 1);
}

// Test with moderate depth
print(countDown(10));
print(countDown(20));
print(countDown(50));

// Mutual recursion with depth tracking
function isEven(int n): int {
    if (n == 0) {
        return 1;
    }
    return isOdd(n - 1);
}

function isOdd(int n): int {
    if (n == 0) {
        return 0;
    }
    return isEven(n - 1);
}

print(isEven(10));
print(isOdd(10));
print(isEven(15));
print(isOdd(15));

// Chain of function calls (not recursive, but deep call stack)
function layer1(int x): int {
    return layer2(x + 1);
}

function layer2(int x): int {
    return layer3(x + 1);
}

function layer3(int x): int {
    return layer4(x + 1);
}

function layer4(int x): int {
    return layer5(x + 1);
}

function layer5(int x): int {
    return layer6(x + 1);
}

function layer6(int x): int {
    return layer7(x + 1);
}

function layer7(int x): int {
    return layer8(x + 1);
}

function layer8(int x): int {
    return layer9(x + 1);
}

function layer9(int x): int {
    return layer10(x + 1);
}

function layer10(int x): int {
    return x + 1;
}

print(layer1(0));

// Fibonacci with depth tracking (moderate depth)
function fib(int n): int {
    if (n <= 1) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

print(fib(10));

// Tail recursive accumulator pattern
function sumToN(int n, int acc): int {
    if (n <= 0) {
        return acc;
    }
    return sumToN(n - 1, acc + n);
}

print(sumToN(10, 0));
print(sumToN(100, 0));

print("Test passed"); // Test completed
