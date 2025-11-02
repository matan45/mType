// Test multiple return statements in single function
function getSign(int num): string {
    if (num > 0) {
        return "positive";
    }
    if (num < 0) {
        return "negative";
    }
    return "zero";
}

function classify(int score): string {
    if (score >= 90) {
        return "A";
    }
    if (score >= 80) {
        return "B";
    }
    if (score >= 70) {
        return "C";
    }
    if (score >= 60) {
        return "D";
    }
    return "F";
}

function findMax(int a, int b, int c): int {
    if (a >= b && a >= c) {
        return a;
    }
    if (b >= a && b >= c) {
        return b;
    }
    return c;
}

print(getSign(5));      // positive
print(getSign(-3));     // negative
print(getSign(0));      // zero

print(classify(95));    // A
print(classify(85));    // B
print(classify(75));    // C
print(classify(65));    // D
print(classify(55));    // F

print(findMax(10, 20, 15));  // 20
print(findMax(30, 5, 25));   // 30
print(findMax(8, 12, 9));    // 12

print("Test passed"); // Test completed
