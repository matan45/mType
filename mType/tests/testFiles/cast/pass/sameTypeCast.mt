// Test: Casting to same type (no-op)
int x = 42;
int y = (int)x;
print(y);  // Expected: 42

float f = 3.14;
float g = (float)f;
print(g);  // Expected: 3.14

bool b = true;
bool c = (bool)b;
print(c);  // Expected: true

string s = "hello";
string t = (string)s;
print(t);  // Expected: hello

// Expected output:
// 42
// 3.14
// true
// hello
