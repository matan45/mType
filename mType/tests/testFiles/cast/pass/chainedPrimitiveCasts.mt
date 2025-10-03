// Test: Chained primitive casts
int x = 42;
float f = (float)x;
string s = (string)f;
print(s);  // Expected: 42.0

bool b = true;
int i = (int)b;
float converted = (float)i;
print(converted);  // Expected: 1.0

// Complex chain
float original = 3.7;
int truncated = (int)original;
bool asBool = (bool)truncated;
int backToInt = (int)asBool;
print(backToInt);  // Expected: 1

// Expected output:
// 42.0
// 1.0
// 1
