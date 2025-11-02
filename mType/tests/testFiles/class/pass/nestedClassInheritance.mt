// Test: Nested class with inheritance
// Expected: Pass - demonstrates inheritance with nested classes

class Base {
    protected int baseValue = 10;

    class BaseNested {
        public void display() {
            print("BaseNested.display(): baseValue=" + Base.this.baseValue);
        }
    }
}

class Derived extends Base {
    private int derivedValue = 20;

    class DerivedNested extends Base.BaseNested {
        public void display() {
            print("DerivedNested.display():");
            print("  baseValue=" + Derived.this.baseValue);
            print("  derivedValue=" + Derived.this.derivedValue);
        }

        public void callSuper() {
            super.display();
        }
    }

    public void test() {
        print("Creating DerivedNested:");
        DerivedNested dn = new DerivedNested();
        dn.display();
        print("\nCalling super:");
        dn.callSuper();
    }
}

// Test nested class inheritance
print("Test 1: Base nested class");
Base b = new Base();
Base.BaseNested bn = new Base.BaseNested();
bn.display();

print("\nTest 2: Derived nested class");
Derived d = new Derived();
d.test();
