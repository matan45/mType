// Test: Int to Bool casting
int zero = 0;
bool b1 = (bool)zero;
print(b1);  // Expected: false

int one = 1;
bool b2 = (bool)one;
print(b2);  // Expected: true

int negative = -1;
bool b3 = (bool)negative;
print(b3);  // Expected: true

int large = 100;
bool b4 = (bool)large;
print(b4);  // Expected: true

// Expected output:
// false
// true
// true
// true
