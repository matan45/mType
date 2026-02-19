// Test: Null to object cast
class MyClass {}

MyClass? obj = null;
MyClass casted = (MyClass)obj;
print(casted == null);

// Expected output:
// true
