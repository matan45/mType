// Test: isClassOf with null
class MyClass {}

MyClass? obj = null;
print(obj isClassOf MyClass); // false

int x = 0;
print(x isClassOf int); // true (0 is not null for primitives)

// Expected output:
// false
// true
