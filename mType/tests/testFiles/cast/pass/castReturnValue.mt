// Test: Cast return value
class Base {
    int id;
    constructor(int i) { this.id = i; }
}

class Derived extends Base {
    constructor(int i) {
	super(i);
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
