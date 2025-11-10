// Test return of function call results
function add(int a, int b): int {
    return a + b;
}

function multiply(int a, int b): int {
    return a * b;
}

function subtract(int a, int b): int {
    return a - b;
}

function divide(int a, int b): int {
    return a / b;
}

// Functions that return results of other functions
function addAndMultiply(int a, int b, int c): int {
    return multiply(add(a, b), c);
}

function complexChain(int x): int {
    return add(multiply(subtract(x, 2), 3), divide(x, 2));
}

function nestedCalls(int a, int b, int c, int d): int {
    return add(subtract(multiply(a, b), divide(c, d)), 10);
}

function factorial(int n): int {
    return n <= 1 ? 1 : multiply(n, factorial(subtract(n, 1)));
}

function sumToN(int n): int {
    return n <= 0 ? 0 : add(n, sumToN(subtract(n, 1)));
}

// Chain method pattern with functions
function processValue(int val): int {
    return add(val, 5);
}

function doubleValue(int val): int {
    return multiply(val, 2);
}

function applyOperations(int val): int {
    return doubleValue(processValue(val));
}

print(addAndMultiply(3, 4, 5));     // 35 (3+4)*5
print(complexChain(10));            // 29 ((10-2)*3) + (10/2)
print(nestedCalls(4, 3, 20, 5));    // 18 ((4*3) - (20/5)) + 10
print(factorial(5));                // 120
print(sumToN(10));                  // 55
print(applyOperations(7));          // 24 (7+5)*2

print("Test passed"); // Test completed
