// Test: instanceof operator for type checking
// Expected: Pass - demonstrates instanceof checks

class Base {
    public function baseMethod(): void {
        print("Base method");
    }
}

class Derived extends Base {
    public function derivedMethod(): void {
        print("Derived method");
    }
}

class Unrelated {
    public function unrelatedMethod(): void {
        print("Unrelated method");
    }
}

// Test instanceof operator
print("Test 1: Base instance");
Base b = new Base();
print("b instanceof Base: " + (b isClassOf Base));
print("b instanceof Derived: " + (b isClassOf Derived));
print("b instanceof Object: " + (b isClassOf Object));

print("\nTest 2: Derived instance");
Derived d = new Derived();
print("d instanceof Base: " + (d isClassOf Base));
print("d instanceof Derived: " + (d isClassOf Derived));
print("d instanceof Object: " + (d isClassOf Object));

print("\nTest 3: Polymorphic reference");
Base polyRef = new Derived();
print("polyRef instanceof Base: " + (polyRef isClassOf Base));
print("polyRef instanceof Derived: " + (polyRef isClassOf Derived));

print("\nTest 4: Unrelated type");
Unrelated u = new Unrelated();
print("u instanceof Base: " + (u isClassOf Base));
print("u instanceof Derived: " + (u isClassOf Derived));
print("u instanceof Unrelated: " + (u isClassOf Unrelated));

print("\nTest 5: Null check");
Base nullRef = null;
print("null instanceof Base: " + (nullRef isClassOf Base));
