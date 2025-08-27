// Test 1: Simple integer ternary
int a = 5;
int b = 3;
int max = a > b ? a : b;
print(max);  // Should print 5
int min = a < b ? a : b;
print(min);  // Should print 3

// Test 2: Simple boolean ternary
bool condition = true;
int result = condition ? 1 : 0;
print(result);  // Should print 1
result = false ? 100 : 200;
print(result);  // Should print 200

// Test 3: Constant expressions
int x = 10 > 5 ? 42 : 99;
print(x);  // Should print 42
int y = 3 < 2 ? 111 : 222;
print(y);  // Should print 222
print("Test passed"); // Test completed