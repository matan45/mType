// Test mutual recursion - functions calling each other recursively
// This tests various scenarios where functions call each other in recursive patterns

// Test 1: Simple mutual recursion with even/odd checking
function isEven(int n): bool {
    if (n == 0) {
        return true;
    }
    if (n < 0) {
        return isOdd(-n);
    }
    return isOdd(n - 1);
}

function isOdd(int n): bool {
    if (n == 0) {
        return false;
    }
    if (n < 0) {
        return isEven(-n);
    }
    return isEven(n - 1);
}

// Test 2: Mutual recursion with mathematical sequence (Fibonacci-like)
function fibA(int n): int {
    if (n <= 0) {
        return 0;
    }
    if (n == 1) {
        return 1;
    }
    return fibB(n - 1) + fibA(n - 2);
}

function fibB(int n): int {
    if (n <= 0) {
        return 1;
    }
    if (n == 1) {
        return 0;
    }
    return fibA(n - 1) + fibB(n - 2);
}

// Test 3: Mutual recursion with string processing
function processStringA(string s,int depth): string {
    if (depth <= 0) {
        return s;
    }
    return processStringB("A:" + s, depth - 1);
}

function processStringB(string s,int depth): string {
    if (depth <= 0) {
        return s;
    }
    return processStringA("B:" + s, depth - 1);
}

// Test 4: Three-way mutual recursion (A -> B -> C -> A)
function countDownA(int n): int {
    if (n <= 0) {
        return 100; // Base case A
    }
    return countDownB(n - 1) + 1;
}

function countDownB(int n): int {
    if (n <= 0) {
        return 200; // Base case B
    }
    return countDownC(n - 1) + 2;
}

function countDownC(int n): int {
    if (n <= 0) {
        return 300; // Base case C
    }
    return countDownA(n - 1) + 3;
}

// Test 5: Mutual recursion with conditional branching
function leftPath(int n,bool direction): int {
    if (n <= 0) {
        return 1;
    }
    if (direction) {
        return rightPath(n - 1, false) * 2;
    } else {
        return rightPath(n - 1, true) + 1;
    }
}

function rightPath(int n,bool direction): int {
    if (n <= 0) {
        return 2;
    }
    if (direction) {
        return leftPath(n - 1, false) + 3;
    } else {
        return leftPath(n - 1, true) * 3;
    }
}

// Execute tests and verify results

print("Testing mutual recursion:");

// Test even/odd functions
print("isEven(4) = " + isEven(4));  // Should be true
print("isOdd(4) = " + isOdd(4));    // Should be false
print("isEven(7) = " + isEven(7));  // Should be false
print("isOdd(7) = " + isOdd(7));    // Should be true
print("isEven(-6) = " + isEven(-6)); // Should be true
print("isOdd(-3) = " + isOdd(-3));   // Should be true

// Test Fibonacci-like sequences
print("fibA(5) = " + fibA(5));
print("fibB(5) = " + fibB(5));
print("fibA(6) = " + fibA(6));
print("fibB(6) = " + fibB(6));

// Test string processing
string result1 = processStringA("start", 3);
print("processStringA(\"start\", 3) = " + result1);

string result2 = processStringB("begin", 2);
print("processStringB(\"begin\", 2) = " + result2);

// Test three-way recursion
print("countDownA(2) = " + countDownA(2));
print("countDownB(2) = " + countDownB(2));
print("countDownC(2) = " + countDownC(2));

// Test conditional mutual recursion
print("leftPath(3, true) = " + leftPath(3, true));
print("rightPath(3, false) = " + rightPath(3, false));
print("leftPath(2, false) = " + leftPath(2, false));
print("rightPath(2, true) = " + rightPath(2, true));

// Test edge cases
print("Edge cases:");
print("isEven(0) = " + isEven(0));   // Should be true
print("isOdd(0) = " + isOdd(0));     // Should be false
print("fibA(0) = " + fibA(0));       // Should be 0
print("fibB(0) = " + fibB(0));       // Should be 1
print("countDownA(0) = " + countDownA(0)); // Should be 100

print("Mutual recursion tests completed successfully");