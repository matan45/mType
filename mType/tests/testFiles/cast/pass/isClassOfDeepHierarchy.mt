// Test: isClassOf with deep hierarchy
class A {}
class B extends A {}
class C extends B {}
class D extends C {}

D obj = new D();
print(obj isClassOf D); // true
print(obj isClassOf C); // true
print(obj isClassOf B); // true
print(obj isClassOf A); // true

// Expected output:
// true
// true
// true
// true
