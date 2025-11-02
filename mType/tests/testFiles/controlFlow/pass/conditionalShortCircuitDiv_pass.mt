// Test short-circuit evaluation preventing division by zero

// Test 1: AND short-circuit with division by zero
int x = 0;
int y = 10;
if (x != 0 && y / x > 0) {
    print(100);
} else {
    print(200);
}

// Test 2: OR short-circuit avoids division
int a = 0;
int b = 5;
if (a == 0 || b / a > 1) {
    print(300);
} else {
    print(400);
}

// Test 3: Multiple short-circuits
int c = 0;
int d = 0;
int e = 10;
if (c != 0 && e / c > 0 && d != 0 && e / d > 0) {
    print(500);
} else {
    print(600);
}

// Test 4: Short-circuit in nested conditions
int f = 0;
int g = 20;
if (f == 0) {
    print(700);
} else {
    if (f != 0 && g / f > 2) {
        print(800);
    }
}

// Test 5: OR chain with multiple potential divisions
int h = 0;
int i = 0;
int j = 30;
if (h == 0 || (h != 0 && j / h > 0) || (i != 0 && j / i > 0)) {
    print(900);
} else {
    print(1000);
}

// Test 6: Complex short-circuit expression
int k = 0;
int m = 5;
int n = 10;
if ((k != 0 && n / k > 0) || (m > 0 && n / m == 2)) {
    print(1100);
} else {
    print(1200);
}

// Test 7: Short-circuit with non-zero values
int p = 2;
int q = 10;
if (p != 0 && q / p == 5) {
    print(1300);
} else {
    print(1400);
}

// Test 8: Ternary with short-circuit
int r = 0;
int s = 15;
int result = r == 0 ? 1500 : s / r;
print(result);

// Test 9: Multiple conditions with safe division
int t = 3;
int u = 12;
if (t != 0 && u / t == 4) {
    print(1600);
}

print("Test passed"); // Test completed
