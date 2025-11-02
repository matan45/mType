// Test multiple conditions with same variables

// Test 1: Redundant but valid checks
int x = 5;
if (x > 0) {
    print(100);
}
if (x > 0) {
    print(200);
}
if (x > 0) {
    print(300);
}

// Test 2: Overlapping conditions
int y = 10;
if (y > 5) {
    print(400);
}
if (y > 8) {
    print(500);
}
if (y > 9) {
    print(600);
}
if (y > 15) {
    print(700);
}

// Test 3: Same condition different branches
int z = 3;
if (z == 3) {
    print(800);
} else {
    print(801);
}
if (z == 3) {
    print(900);
} else {
    print(901);
}

// Test 4: Redundant checks in nested structure
int a = 7;
if (a > 5) {
    if (a > 5) {
        if (a > 5) {
            print(1000);
        }
    }
}

// Test 5: Multiple checks with same variables but different operators
int b = 15;
if (b > 10) {
    print(1100);
}
if (b >= 15) {
    print(1200);
}
if (b < 20) {
    print(1300);
}
if (b <= 15) {
    print(1400);
}
if (b == 15) {
    print(1500);
}
if (b != 10) {
    print(1600);
}

// Test 6: Redundant boolean checks
bool flag = true;
if (flag) {
    print(1700);
}
if (flag == true) {
    print(1800);
}
if (flag != false) {
    print(1900);
}

print("Test passed"); // Test completed
