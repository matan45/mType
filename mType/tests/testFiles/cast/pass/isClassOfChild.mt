// Test: isClassOf with child class
class Base {}
class Derived extends Base {}

Base b = new Derived();
print(b isClassOf Derived); // true
print(b isClassOf Base);    // true

// Expected output:
// true
// true
