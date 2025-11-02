// Test: Inner class accessing outer class members
// Expected: Pass - demonstrates outer member access from inner class

class Container {
    private int value;
    private string name;

    public constructor(string name, int value) {
        this.name = name;
        this.value = value;
        print("Container created: " + name);
    }

    class Iterator {
        private int position;

        public constructor() {
            this.position = 0;
            print("Iterator created for container: " + Container.this.name);
        }

        public void displayOuter() {
            print("Outer name: " + Container.this.name);
            print("Outer value: " + Container.this.value);
            print("Position: " + this.position);
        }

        public void advance() {
            this.position = this.position + 1;
            print("Advanced to position: " + this.position);
        }
    }

    public Iterator createIterator() {
        return new Iterator();
    }
}

// Test inner class accessing outer members
print("Test 1: Create container");
Container c = new Container("MyContainer", 42);

print("\nTest 2: Create iterator");
Container.Iterator iter = c.createIterator();
iter.displayOuter();

print("\nTest 3: Advance and display");
iter.advance();
iter.advance();
iter.displayOuter();
