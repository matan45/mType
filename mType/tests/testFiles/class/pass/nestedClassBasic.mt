// Test: Basic nested class definition and usage
// Expected: Pass - demonstrates nested classes

class Outer {
    private int outerValue = 10;

    public constructor() {
        print("Outer constructor");
    }

    class Inner {
        private int innerValue = 20;

        public constructor() {
            print("Inner constructor");
        }

        public void display() {
            print("Inner.display(): innerValue=" + this.innerValue);
        }

        public int getInnerValue() {
            return this.innerValue;
        }
    }

    public void createInner() {
        print("Creating Inner from Outer");
        Inner i = new Inner();
        i.display();
    }

    public int getOuterValue() {
        return this.outerValue;
    }
}

// Test nested classes
print("Test 1: Create outer");
Outer o = new Outer();
print("Outer value: " + o.getOuterValue());

print("\nTest 2: Create inner from outer");
o.createInner();

print("\nTest 3: Direct inner creation");
Outer.Inner inner = new Outer.Inner();
inner.display();
print("Inner value: " + inner.getInnerValue());
