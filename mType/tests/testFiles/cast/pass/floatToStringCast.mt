// Test: Float to String casting
float f = 3.14;
string s = (string)f;
print(s);  // Expected: 3.14

float negative = -2.5;
string negStr = (string)negative;
print(negStr);  // Expected: -2.5

float zero = 0.0;
string zeroStr = (string)zero;
print(zeroStr);  // Expected: 0.0

// Expected output:
// 3.14
// -2.5
// 0.0
