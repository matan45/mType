// Test if without else with side effects

// Test 1: Simple if without else
int x = 5;
if (x > 0) {
    print(100);
}
print(200);

// Test 2: If without else, condition false
int y = 3;
if (y > 10) {
    print(300);
}
print(400);

// Test 3: Multiple if statements without else
int z = 7;
if (z > 5) {
    print(500);
}
if (z < 10) {
    print(600);
}
if (z == 7) {
    print(700);
}
print(800);

// Test 4: If without else modifying variables
int counter = 0;
if (true) {
    counter = counter + 1;
}
print(counter);

if (false) {
    counter = counter + 10;
}
print(counter);

if (counter > 0) {
    counter = counter + 100;
}
print(counter);

// Test 5: Nested if without else
int a = 10;
if (a > 5) {
    print(900);
    if (a > 8) {
        print(1000);
        if (a == 10) {
            print(1100);
        }
    }
}
print(1200);

// Test 6: If without else in sequence
int b = 15;
if (b > 10) {
    print(1300);
}
print(1400);
if (b < 20) {
    print(1500);
}
print(1600);
if (b == 15) {
    print(1700);
}
print(1800);

// Test 7: If without else with complex expressions
int c = 8;
int d = 4;
if (c + d == 12) {
    print(1900);
}
print(2000);

if (c * d > 30) {
    print(2100);
}
print(2200);

if (c / d == 2) {
    print(2300);
}
print(2400);

// Test 8: Empty if without else
if (false) {
}
print(2500);

if (true) {
}
print(2600);

print("Test passed"); // Test completed
