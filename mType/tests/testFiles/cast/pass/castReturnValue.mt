// Test: Cast return value
class Base {
    int id;
    Base(int i) { this.id = i; }
}

class Derived extends Base {
    Derived(int i) : super(i) {}
}

Base getBase() {
    return new Derived(42);
}

Base b = getBase();
Derived d = (Derived)b;  // Cast return value
print(d.id);  // Expected: 42

// Expected output:
// 42
