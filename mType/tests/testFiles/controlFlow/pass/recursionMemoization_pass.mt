// Test recursion with memoization/caching pattern
// Using arrays to cache computed values

// Fibonacci with memoization
int[] fibCache = [0, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1];

function fibMemo(int n): int {
    if (n < 0) {
        return 0;
    }
    if (n >= 20) {
        // Beyond cache, compute directly
        return fibMemo(n - 1) + fibMemo(n - 2);
    }

    if (fibCache[n] != -1) {
        return fibCache[n];
    }

    int result = fibMemo(n - 1) + fibMemo(n - 2);
    fibCache[n] = result;
    return result;
}

// Tribonacci with memoization (sum of previous 3 numbers)
int[] tribCache = [-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1];

function tribMemo(int n): int {
    if (n == 0) {
        return 0;
    }
    if (n == 1 || n == 2) {
        return 1;
    }

    if (n >= 15) {
        return tribMemo(n - 1) + tribMemo(n - 2) + tribMemo(n - 3);
    }

    if (tribCache[n] != -1) {
        return tribCache[n];
    }

    int result = tribMemo(n - 1) + tribMemo(n - 2) + tribMemo(n - 3);
    tribCache[n] = result;
    return result;
}

// Count ways to climb stairs (can climb 1 or 2 steps at a time)
int[] stairsCache = [-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1];

function countWays(int n): int {
    if (n <= 1) {
        return 1;
    }

    if (n >= 12) {
        return countWays(n - 1) + countWays(n - 2);
    }

    if (stairsCache[n] != -1) {
        return stairsCache[n];
    }

    int result = countWays(n - 1) + countWays(n - 2);
    stairsCache[n] = result;
    return result;
}

print("Testing recursion with memoization:");

// Test Fibonacci with memoization
print("fibMemo(10) = " + fibMemo(10));     // 55
print("fibMemo(15) = " + fibMemo(15));     // 610
print("fibMemo(19) = " + fibMemo(19));     // 4181

// Test Tribonacci
print("tribMemo(8) = " + tribMemo(8));     // 44
print("tribMemo(10) = " + tribMemo(10));   // 149
print("tribMemo(12) = " + tribMemo(12));   // 504

// Test stairs climbing
print("countWays(5) = " + countWays(5));   // 8
print("countWays(8) = " + countWays(8));   // 34
print("countWays(10) = " + countWays(10)); // 89

// Verify cache is being used by calling again
print("fibMemo(10) again = " + fibMemo(10));     // 55 (from cache)
print("tribMemo(10) again = " + tribMemo(10));   // 149 (from cache)

print("Memoization tests completed successfully");
