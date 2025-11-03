// Test: Static method hiding vs instance method overriding
// Expected: Pass - demonstrates difference between hiding and overriding

class Base {
    // Instance method - can be overridden
    public function instanceMethod(): void {
        print("Base.instanceMethod()");
    }

    // Static method - can be hidden
    public static function staticMethod(): void {
        print("Base.staticMethod()");
    }

    public function callBoth(): void {
        print("Base.callBoth():");
        this.instanceMethod();
        Base::staticMethod();
    }
}

class Derived extends Base {
    // Override instance method
    public function instanceMethod(): void {
        print("Derived.instanceMethod()");
    }

    // Hide static method (not override)
    public static function staticMethod(): void {
        print("Derived.staticMethod()");
    }

    public function callBoth(): void {
        print("Derived.callBoth():");
        this.instanceMethod();
        Derived::staticMethod();
    }
}

// Test method hiding vs overriding
print("Test 1: Direct calls on Derived");
Derived d = new Derived();
d.instanceMethod();
Derived::staticMethod();

print("\nTest 2: Direct calls on Base");
Base b = new Base();
b.instanceMethod();
Base::staticMethod();

print("\nTest 3: Polymorphic instance method (overriding)");
Base polyBase = new Derived();
polyBase.instanceMethod();  // Calls Derived version (polymorphic)

print("\nTest 4: Static method via reference (hiding)");
Base::staticMethod();        // Calls Base version
Derived::staticMethod();     // Calls Derived version

print("\nTest 5: callBoth method");
b.callBoth();
print();
d.callBoth();
print();
polyBase.callBoth();
