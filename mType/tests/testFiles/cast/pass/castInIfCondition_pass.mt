// Test: Casting in if condition
class Base {
    public int value;

    constructor(int v) {
        this.value = v;
    }
}

class Derived extends Base {
    public int multiplier;

    constructor(int v, int m):super(v) {
        this.multiplier = m;
    }

    public function getTotal(): int {
        return this.value * this.multiplier;
    }
}

Base obj = new Derived(10, 3);

// Cast in if condition - check if cast result has high value
if (((Derived)obj).getTotal() > 25) {
    print("High value");
} else {
    print("Low value");
}

// Multiple casts in compound condition
Base obj2 = new Derived(5, 2);
if (((Derived)obj).value > 5 && ((Derived)obj2).multiplier == 2) {
    print("Both conditions met");
}

// Cast with type check in condition
if (obj isClassOf Derived && ((Derived)obj).multiplier > 1) {
    print("Safe cast succeeded");
}

// Expected output:
// High value
// Both conditions met
// Safe cast succeeded
