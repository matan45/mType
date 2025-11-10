// Test recursion with complex termination conditions
// Multiple base cases and conditional recursion paths

// Collatz sequence - multiple base cases and paths
function collatzSteps(int n): int {
    if (n <= 0) {
        return 0; // Invalid input
    }
    if (n == 1) {
        return 0; // Base case - reached 1
    }
    if (n % 2 == 0) {
        return 1 + collatzSteps(n / 2); // Even: divide by 2
    } else {
        return 1 + collatzSteps(3 * n + 1); // Odd: multiply by 3 and add 1
    }
}

// Ackermann function - multiple base cases
function ackermann(int m, int n): int {
    if (m == 0) {
        return n + 1; // Base case 1
    }
    if (m > 0 && n == 0) {
        return ackermann(m - 1, 1); // Base case 2
    }
    if (m > 0 && n > 0) {
        return ackermann(m - 1, ackermann(m, n - 1)); // Recursive case
    }
    return 0; // Should not reach
}

// Range check with multiple conditions
function rangeSum(int start, int end, int step): int {
    if (step <= 0) {
        return 0; // Invalid step
    }
    if (start > end) {
        return 0; // Base case - range exhausted
    }
    if (start == end) {
        return start; // Base case - single element
    }
    return start + rangeSum(start + step, end, step);
}

// Digit-based recursion with multiple conditions
function digitalRoot(int n): int {
    if (n < 0) {
        return digitalRoot(-n); // Handle negative
    }
    if (n < 10) {
        return n; // Base case - single digit
    }

    // Sum digits and recurse
    int sum = 0;
    int temp = n;
    while (temp > 0) {
        sum = sum + (temp % 10);
        temp = temp / 10;
    }
    return digitalRoot(sum);
}

// Bounded search with multiple exit conditions
function findDivisor(int n, int candidate, int limit): int {
    if (candidate > limit) {
        return -1; // Not found within limit
    }
    if (candidate * candidate > n) {
        return -1; // Exceeded square root
    }
    if (n % candidate == 0) {
        return candidate; // Found divisor
    }
    return findDivisor(n, candidate + 1, limit);
}

// String-based recursion with multiple base cases
function countVowels(string s, int index, int len): int {
    if (index >= len) {
        return 0; // Base case - end of string
    }
    if (len == 0) {
        return 0; // Base case - empty string
    }

    // Note: Simplified check - would need charAt in real implementation
    // For testing, we'll use a different approach
    return countVowels(s, index + 1, len);
}

print("Testing recursion with multiple base cases:");

// Test Collatz sequence
print("collatzSteps(1) = " + collatzSteps(1));     // 0
print("collatzSteps(2) = " + collatzSteps(2));     // 1
print("collatzSteps(3) = " + collatzSteps(3));     // 7
print("collatzSteps(16) = " + collatzSteps(16));   // 4
print("collatzSteps(27) = " + collatzSteps(27));   // 111

// Test Ackermann function (small values only - grows very fast)
print("ackermann(0, 0) = " + ackermann(0, 0));     // 1
print("ackermann(1, 0) = " + ackermann(1, 0));     // 2
print("ackermann(1, 1) = " + ackermann(1, 1));     // 3
print("ackermann(2, 2) = " + ackermann(2, 2));     // 7
print("ackermann(3, 2) = " + ackermann(3, 2));     // 29

// Test range sum
print("rangeSum(1, 10, 1) = " + rangeSum(1, 10, 1));     // 55
print("rangeSum(1, 10, 2) = " + rangeSum(1, 10, 2));     // 25
print("rangeSum(5, 5, 1) = " + rangeSum(5, 5, 1));       // 5
print("rangeSum(10, 5, 1) = " + rangeSum(10, 5, 1));     // 0
print("rangeSum(1, 20, 3) = " + rangeSum(1, 20, 3));     // 49

// Test digital root
print("digitalRoot(0) = " + digitalRoot(0));       // 0
print("digitalRoot(9) = " + digitalRoot(9));       // 9
print("digitalRoot(38) = " + digitalRoot(38));     // 2
print("digitalRoot(65) = " + digitalRoot(65));     // 2
print("digitalRoot(99) = " + digitalRoot(99));     // 9

// Test divisor finding
print("findDivisor(15, 2, 10) = " + findDivisor(15, 2, 10));    // 3
print("findDivisor(17, 2, 10) = " + findDivisor(17, 2, 10));    // -1
print("findDivisor(100, 2, 20) = " + findDivisor(100, 2, 20));  // 2
print("findDivisor(21, 2, 10) = " + findDivisor(21, 2, 10));    // 3

print("Multiple base case tests completed successfully");
