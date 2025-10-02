// Test: Null cast in conditional
class MyClass {}

MyClass obj = null;
if ((MyClass)obj == null) {
    print("Cast preserves null");
}

// Expected output:
// Cast preserves null
