// Test: isClassOf with primitives
int x = 42;
print(x isClassOf int);    // true
print(x isClassOf float);  // false
print(x isClassOf string); // false

float f = 3.14;
print(f isClassOf float);  // true
print(f isClassOf int);    // false

// Expected output:
// true
// false
// false
// true
// false
