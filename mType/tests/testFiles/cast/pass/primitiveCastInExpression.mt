// Test: Primitive cast in expressions
int a = 10;
int b = 3;
float result = (float)a / (float)b;
print(result);  // Expected: 3.333333 (or similar)

// Cast in comparison
float f = 5.5;
bool compare = (int)f > 5;
print(compare);  // Expected: false

// Cast in arithmetic
int x = 7;
float doubled = (float)x * 2.0;
print(doubled);  // Expected: 14.0

// Expected output:
// 3.333333
// false
// 14.0
