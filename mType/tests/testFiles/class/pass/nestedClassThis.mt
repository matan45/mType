// Test: 'this' keyword disambiguation in nested classes
// Expected: Pass - demonstrates this vs OuterClass.this

class Outer {
    private int value = 100;

    public constructor() {
        print("Outer constructor, value=" + this.value);
    }

    class Inner {
        private int value = 200;

        public constructor() {
            print("Inner constructor, value=" + this.value);
        }

        public void displayBoth() {
            print("Inner.displayBoth():");
            print("  this.value (inner) = " + this.value);
            print("  Outer.this.value (outer) = " + Outer.this.value);
        }

        public void modifyBoth(int innerVal, int outerVal) {
            print("Modifying values:");
            this.value = innerVal;
            Outer.this.value = outerVal;
            print("  this.value (inner) = " + this.value);
            print("  Outer.this.value (outer) = " + Outer.this.value);
        }
    }

    public int getValue() {
        return this.value;
    }

    public Inner createInner() {
        return new Inner();
    }
}

// Test 'this' in nested classes
print("Test 1: Create outer and inner");
Outer o = new Outer();
Outer.Inner i = o.createInner();
i.displayBoth();

print("\nTest 2: Modify both values");
i.modifyBoth(300, 400);
print("Outer value from outside: " + o.getValue());

print("\nTest 3: Display again");
i.displayBoth();
