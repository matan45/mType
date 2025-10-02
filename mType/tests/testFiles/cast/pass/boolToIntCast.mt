// Test: Bool to Int casting
bool t = true;
int x = (int)t;
print(x);  // Expected: 1

bool f = false;
int y = (int)f;
print(y);  // Expected: 0

// Use in expression
int sum = (int)true + (int)false + (int)true;
print(sum);  // Expected: 2

// Expected output:
// 1
// 0
// 2
