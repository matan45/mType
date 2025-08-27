function min(int a, int b): int {
    if (a < b) {
        return a;
    }
    return b;
}

function max(int a, int b): int {
    if (a > b) {
        return a;
    }
    return b;
}

function clamp(int value, int minVal, int maxVal): int {
    return min(max(value, minVal), maxVal);
}

function abs(int x): int {
    if (x < 0) {
        return -x;
    }
    return x;
}

function distance(int x1, int y1, int x2, int y2): int {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    return dx + dy;  // Manhattan distance
}

print(clamp(15, 0, 10));   // 10
print(clamp(-5, 0, 10));   // 0
print(clamp(5, 0, 10));    // 5

print(distance(0, 0, 3, 4));  // 7 (Manhattan distance)
print("Test passed"); // Test completed