// Test: isClassOf with multiple interfaces
interface I1 {}
interface I2 {}
class C implements I1, I2 {}

C obj = new C();
print(obj isClassOf C);  // true
print(obj isClassOf I1); // true
print(obj isClassOf I2); // true

// Expected output:
// true
// true
// true
