// Test: instanceof operator for type checking
// Expected: Pass - demonstrates instanceof checks

class Base {
    public void baseMethod() {
        print("Base method");
    }
}

class Derived extends Base {
    public void derivedMethod() {
        print("Derived method");
    }
}

class Unrelated {
    public void unrelatedMethod() {
        print("Unrelated method");
    }
}

// Test instanceof operator
print("Test 1: Base instance");
Base b = new Base();
print("b instanceof Base: " + (b instanceof Base));
print("b instanceof Derived: " + (b instanceof Derived));
print("b instanceof Object: " + (b instanceof Object));

print("\nTest 2: Derived instance");
Derived d = new Derived();
print("d instanceof Base: " + (d instanceof Base));
print("d instanceof Derived: " + (d instanceof Derived));
print("d instanceof Object: " + (d instanceof Object));

print("\nTest 3: Polymorphic reference");
Base polyRef = new Derived();
print("polyRef instanceof Base: " + (polyRef instanceof Base));
print("polyRef instanceof Derived: " + (polyRef instanceof Derived));

print("\nTest 4: Unrelated type");
Unrelated u = new Unrelated();
print("u instanceof Base: " + (u instanceof Base));
print("u instanceof Derived: " + (u instanceof Derived));
print("u instanceof Unrelated: " + (u instanceof Unrelated));

print("\nTest 5: Null check");
Base nullRef = null;
print("null instanceof Base: " + (nullRef instanceof Base));
