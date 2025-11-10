// Test: Null cast in complex expressions
class Value {
    public int data;

    constructor(int d) {
        this.data = d;
    }
}

class Base {}
class Derived extends Base {
    public int value;

    constructor(int v) {
        this.value = v;
    }
}

// Cast in arithmetic expression with null handling
Value v1 = new Value(10);
Value v2 = null;
Value v3 = new Value(5);

int sum = ((v1 != null) ? (Value)v1 : new Value(0)).data +
          ((v2 != null) ? (Value)v2 : new Value(0)).data +
          ((v3 != null) ? (Value)v3 : new Value(0)).data;
print(sum);

// Cast in boolean expression
Base b1 = new Derived(100);
Base b2 = null;

bool result1 = (b1 != null) && ((Derived)(Base)b1).value > 50;
bool result2 = (b2 != null) && ((Derived)(Base)b2).value > 50;
print(result1);
print(result2);

// Nested cast with null
Derived d = new Derived(42);
Base b = (Base)d;
Derived d2 = b != null ? (Derived)(Base)b : null;
print(d2 != null);

// Expected output:
// 15
// true
// false
// true
