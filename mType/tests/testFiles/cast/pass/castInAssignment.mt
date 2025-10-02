// Test: Cast in assignment
class Base {
    int value;
    Base(int v) { this.value = v; }
}

class Derived extends Base {
    Derived(int v) : super(v) {}
}

Derived d1 = new Derived(10);
Derived d2 = new Derived(20);

// Cast in assignment
Base b = (Base)d1;
print(b.value);  // Expected: 10

// Multiple assignments with casts
b = (Base)d2;
print(b.value);  // Expected: 20

// Expected output:
// 10
// 20
