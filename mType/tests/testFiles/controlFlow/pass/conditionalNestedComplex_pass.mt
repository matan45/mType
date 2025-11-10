// Test deeply nested if-else structures

// Test 1: Deep nesting with all branches taken at different levels
int level1 = 1;
if (level1 == 1) {
    print(100);
    int level2 = 2;
    if (level2 == 2) {
        print(200);
        int level3 = 3;
        if (level3 == 3) {
            print(300);
            int level4 = 4;
            if (level4 == 4) {
                print(400);
                int level5 = 5;
                if (level5 == 5) {
                    print(500);
                }
            }
        }
    }
}

// Test 2: Deep nesting with else branches
int x = 0;
if (x > 0) {
    print(600);
} else {
    print(700);
    if (x < 0) {
        print(800);
    } else {
        print(900);
        if (x == 0) {
            print(1000);
        }
    }
}

// Test 3: Complex nested structure with multiple conditions
int a = 5;
int b = 10;
if (a > 0) {
    print(1100);
    if (b > 5) {
        print(1200);
        if (a + b == 15) {
            print(1300);
            if (a < b) {
                print(1400);
            } else {
                print(1500);
            }
        } else {
            print(1600);
        }
    } else {
        print(1700);
    }
} else {
    print(1800);
}

// Test 4: Nested with mixed if and else-if
int c = 3;
if (c > 0) {
    print(1900);
    if (c > 5) {
        print(2000);
    } else if (c > 2) {
        print(2100);
        if (c == 3) {
            print(2200);
        } else if (c == 4) {
            print(2300);
        } else {
            print(2400);
        }
    } else {
        print(2500);
    }
} else {
    print(2600);
}

// Test 5: Deep nesting with variable modifications
int d = 0;
if (true) {
    d = d + 1;
    print(d);
    if (true) {
        d = d + 1;
        print(d);
        if (true) {
            d = d + 1;
            print(d);
            if (true) {
                d = d + 1;
                print(d);
            }
        }
    }
}

// Test 6: Asymmetric deep nesting
int e = 10;
if (e > 5) {
    print(2700);
    if (e > 8) {
        print(2800);
        if (e >= 10) {
            print(2900);
        }
    }
} else {
    print(3000);
    if (e < 3) {
        print(3100);
        if (e < 1) {
            print(3200);
            if (e < 0) {
                print(3300);
            }
        }
    }
}

// Test 7: Deep nesting with complex boolean expressions
int f = 7;
int g = 3;
if (f > g) {
    print(3400);
    if (f > g && f > 5) {
        print(3500);
        if (f > g && f > 5 && g < 5) {
            print(3600);
            if (f - g == 4) {
                print(3700);
            }
        }
    }
}

print("Test passed"); // Test completed
