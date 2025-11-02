// Test: Null safety and type checking
// Expected: Pass - demonstrates null handling patterns

class Container {
    private Object value;

    public constructor() {
        this.value = null;
    }

    public void setValue(Object value) {
        this.value = value;
        if (value == null) {
            print("Value set to null");
        } else {
            print("Value set to: " + value);
        }
    }

    public Object getValue() {
        return this.value;
    }

    public bool hasValue() {
        return this.value != null;
    }

    public void safeProcess() {
        if (this.value == null) {
            print("No value to process");
            return;
        }
        print("Processing value: " + this.value);
    }
}

class DataHolder {
    private string data;

    public constructor(string data) {
        this.data = data;
    }

    public string getData() {
        return this.data;
    }
}

// Test null safety
print("Test 1: Null initialization");
Container c = new Container();
print("Has value: " + c.hasValue());
c.safeProcess();

print("\nTest 2: Set non-null value");
c.setValue(new DataHolder("Test Data"));
print("Has value: " + c.hasValue());
c.safeProcess();

print("\nTest 3: Set back to null");
c.setValue(null);
print("Has value: " + c.hasValue());
c.safeProcess();

print("\nTest 4: Type checking with null");
Object obj = null;
print("null instanceof Object: " + (obj instanceof Object));
obj = new DataHolder("Data");
print("non-null instanceof Object: " + (obj instanceof Object));

print("\nTest 5: Null comparison");
Container c1 = new Container();
Container c2 = new Container();
c1.setValue(null);
c2.setValue(null);
print("Both null, same value: " + (c1.getValue() == c2.getValue()));
