// Test: Cast return value
class Base {
    public int id;
    public constructor(int i) { this.id = i; }
}

class Derived extends Base {
    public constructor(int i):super(i) {
	}
}

function getBase(): Base {
    return new Derived(42);
}

Base b = getBase();
Derived d = (Derived)b;  // Cast return value
print(d.id);  // Expected: 42

// Expected output:
// 42
