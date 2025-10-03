// Test: Float to Int casting (truncation)
float f = 3.14;
int x = (int)f;
print(x);  // Expected: 3

float large = 99.99;
int truncated = (int)large;
print(truncated);  // Expected: 99

float negative = -5.7;
int negInt = (int)negative;
print(negInt);  // Expected: -5

// Expected output:
// 3
// 99
// -5
