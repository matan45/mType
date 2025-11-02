// Test: Final reference to mutable object
// Expected: Pass - final prevents reassignment but not mutation

class MutableData {
    public int value;

    public constructor(int value) {
        this.value = value;
    }
}

class Container {
    // Final reference - cannot be reassigned but object can be mutated
    public final MutableData data;

    public constructor(int initialValue) {
        this.data = new MutableData(initialValue);
        print("Container created with value: " + this.data.value);
    }

    public void mutateData(int newValue) {
        // This is allowed - mutating the object
        print("Mutating from " + this.data.value + " to " + newValue);
        this.data.value = newValue;
    }

    public void display() {
        print("Current value: " + this.data.value);
    }
}

// Test final mutable reference
print("Test 1: Create container");
Container c = new Container(10);
c.display();

print("\nTest 2: Mutate object");
c.mutateData(20);
c.display();

print("\nTest 3: Multiple mutations");
c.mutateData(30);
c.mutateData(40);
c.display();

print("\nTest 4: Direct mutation");
c.data.value = 50;
print("After direct mutation: " + c.data.value);
