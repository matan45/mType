// Test: Cast to same class (no-op)
class MyClass {
    public int value;

    public constructor(int v) {
        this.value = v;
    }
}

MyClass obj1 = new MyClass(42);
MyClass obj2 = (MyClass)obj1;
print(obj2.value);  // Expected: 42

// Expected output:
// 42
