// Test: Final field initialization constraints
// Expected: Pass - final fields must be initialized

class FinalFields {
    // Final field with inline initialization
    public final int CONSTANT = 100;

    // Final field initialized in constructor
    public final string name;
    public final int id;

    private static int nextId = 0;

    public constructor(string name) {
        this.id = FinalFields.nextId;
        FinalFields.nextId = FinalFields.nextId + 1;
        this.name = name;
        print("Created: " + this.name + " with id=" + this.id);
    }

    public void display() {
        print("Object: " + this.name + ", id=" + this.id + ", constant=" + this.CONSTANT);
    }
}

// Test final field initialization
print("Test 1: Create first object");
FinalFields obj1 = new FinalFields("First");
obj1.display();

print("\nTest 2: Create second object");
FinalFields obj2 = new FinalFields("Second");
obj2.display();

print("\nTest 3: Create third object");
FinalFields obj3 = new FinalFields("Third");
obj3.display();
