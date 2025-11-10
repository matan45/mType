// Test very deep recursion causing stack overflow
// This should trigger a stack overflow or maximum recursion depth error

// Infinite recursion - no base case
function infiniteRecursion(int n): int {
    return infiniteRecursion(n + 1);
}

// Very deep recursion that will exceed stack limits
function veryDeepRecursion(int n): int {
    if (n <= 0) {
        return 0;
    }
    return 1 + veryDeepRecursion(n - 1);
}

// Mutual infinite recursion
function mutualA(int n): int {
    return mutualB(n + 1);
}

function mutualB(int n): int {
    return mutualA(n + 1);
}

print("Testing stack overflow detection:");

// This should cause stack overflow - calling with very large number
// The VM should detect this and throw an error before actual stack overflow
print("Attempting very deep recursion...");
int result = veryDeepRecursion(100000);  // ERROR: Stack overflow
print("Should not reach here: " + result);
