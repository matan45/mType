// Test: Static nested class
// Expected: Pass - demonstrates static nested classes

class OuterClass {
    private int instanceValue = 10;
    private static int staticValue = 20;

    public constructor() {
        print("OuterClass constructor");
    }

    static class StaticNested {
        private int nestedValue = 30;

        public constructor() {
            print("StaticNested constructor");
        }

        public void display() {
            print("StaticNested.display():");
            print("  nestedValue=" + this.nestedValue);
            print("  outer static=" + OuterClass.staticValue);
            // Cannot access OuterClass.instanceValue - no outer instance
        }

        public static void staticMethod() {
            print("StaticNested.staticMethod()");
            print("  outer static=" + OuterClass.staticValue);
        }
    }

    public void createNested() {
        print("Creating StaticNested from outer");
        StaticNested sn = new StaticNested();
        sn.display();
    }
}

// Test static nested class
print("Test 1: Create via outer class");
OuterClass o = new OuterClass();
o.createNested();

print("\nTest 2: Direct creation (no outer instance needed)");
OuterClass.StaticNested sn = new OuterClass.StaticNested();
sn.display();

print("\nTest 3: Static method call");
OuterClass.StaticNested.staticMethod();
