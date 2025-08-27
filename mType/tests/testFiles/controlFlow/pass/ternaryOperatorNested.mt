// Test 1: Simple nested ternary
int x = 1;
int y = 2;
int z = 3;
int max = x > y ? (x > z ? x : z) : (y > z ? y : z);
print(max);  // Should print 3

// Test 2: Deep nesting
int a = 10;
int b = 20;
int c = 15;
int d = 25;

int result = a > b ? (a > c ? a : c) : (b > c ? (b > d ? b : d) : (c > d ? c : d));
print(result);  // Should print 25
print("Test passed"); // Test completed