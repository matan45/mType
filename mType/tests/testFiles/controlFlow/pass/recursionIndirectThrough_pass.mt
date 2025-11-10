// Test indirect recursion through another function
// Functions call each other in chains or cycles

// Simple indirect recursion: A -> helper -> A
function processValue(int n): int {
    if (n <= 0) {
        return 0;
    }
    return processHelper(n);
}

function processHelper(int n): int {
    if (n == 1) {
        return 1;
    }
    return n + processValue(n - 1);
}

// Three-function chain: A -> B -> C -> A
function chainA(int n): int {
    if (n <= 0) {
        return 100;
    }
    return chainB(n);
}

function chainB(int n): int {
    if (n == 1) {
        return 200;
    }
    return 1 + chainC(n);
}

function chainC(int n): int {
    if (n == 2) {
        return 300;
    }
    return 2 + chainA(n - 1);
}

// Indirect recursion with conditional routing
function routeEven(int n): int {
    if (n <= 0) {
        return 0;
    }
    if (n % 2 == 0) {
        return n + routeOdd(n - 1);
    }
    return routeOdd(n);
}

function routeOdd(int n): int {
    if (n <= 0) {
        return 0;
    }
    if (n % 2 != 0) {
        return n + routeEven(n - 1);
    }
    return routeEven(n);
}

// Calculation through intermediary
function calculate(int n, int multiplier): int {
    if (n <= 0) {
        return 0;
    }
    return applyMultiplier(n, multiplier);
}

function applyMultiplier(int n, int m): int {
    if (n == 1) {
        return m;
    }
    return m + calculate(n - 1, m);
}

// Tree-like indirect recursion
function splitLeft(int n, int depth): int {
    if (depth <= 0 || n <= 0) {
        return n;
    }
    return splitRight(n - 1, depth - 1) + 1;
}

function splitRight(int n, int depth): int {
    if (depth <= 0 || n <= 0) {
        return n;
    }
    return splitLeft(n - 1, depth - 1) + 2;
}

// Ping-pong recursion with accumulator
function ping(int n, int acc): int {
    if (n <= 0) {
        return acc;
    }
    return pong(n - 1, acc + n);
}

function pong(int n, int acc): int {
    if (n <= 0) {
        return acc;
    }
    return ping(n - 1, acc + n);
}

print("Testing indirect recursion through other functions:");

// Test simple indirect recursion
print("processValue(5) = " + processValue(5));     // 15
print("processValue(10) = " + processValue(10));   // 55

// Test three-function chain
print("chainA(1) = " + chainA(1));                 // 200
print("chainA(2) = " + chainA(2));                 // 300
print("chainA(3) = " + chainA(3));                 // 203
print("chainA(4) = " + chainA(4));                 // 303

// Test conditional routing
print("routeEven(5) = " + routeEven(5));           // 15
print("routeEven(10) = " + routeEven(10));         // 55
print("routeOdd(6) = " + routeOdd(6));             // 21
print("routeOdd(9) = " + routeOdd(9));             // 45

// Test calculation through intermediary
print("calculate(5, 3) = " + calculate(5, 3));     // 15
print("calculate(7, 2) = " + calculate(7, 2));     // 14
print("calculate(4, 10) = " + calculate(4, 10));   // 40

// Test tree-like recursion
print("splitLeft(5, 3) = " + splitLeft(5, 3));     // 9
print("splitRight(5, 3) = " + splitRight(5, 3));   // 8
print("splitLeft(7, 2) = " + splitLeft(7, 2));     // 8
print("splitRight(6, 2) = " + splitRight(6, 2));   // 7

// Test ping-pong recursion
print("ping(5, 0) = " + ping(5, 0));               // 15
print("pong(5, 0) = " + pong(5, 0));               // 15
print("ping(10, 0) = " + ping(10, 0));             // 55
print("pong(10, 0) = " + pong(10, 0));             // 55

print("Indirect recursion tests completed successfully");
