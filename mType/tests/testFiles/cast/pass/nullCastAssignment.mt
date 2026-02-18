// Test: Null cast in assignment
class Base {}
class Derived extends Base {}

Derived? d = null;
Base b = (Base)d;
print(b == null);

// Expected output:
// true
