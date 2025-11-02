// Test: Default values for uninitialized fields
// Expected: Pass - demonstrates default field initialization

class Defaults {
    // Uninitialized fields should have default values
    public int intField;
    public bool boolField;
    public string stringField;
    public Object objectField;

    public void display() {
        print("intField: " + this.intField);
        print("boolField: " + this.boolField);
        print("stringField: " + this.stringField);
        print("objectField: " + this.objectField);
    }

    public void initialize() {
        this.intField = 42;
        this.boolField = true;
        this.stringField = "initialized";
        this.objectField = new Object();
        print("Fields initialized");
    }
}

// Test default field values
print("Test 1: Uninitialized fields");
Defaults obj1 = new Defaults();
obj1.display();

print("\nTest 2: After initialization");
obj1.initialize();
obj1.display();

print("\nTest 3: New object");
Defaults obj2 = new Defaults();
obj2.display();
