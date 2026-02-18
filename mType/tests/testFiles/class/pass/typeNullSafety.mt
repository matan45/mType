// Test: Null safety and type checking
// Expected: Pass - demonstrates null handling patterns

class Object{}

class Container {
    private Object value;

    public constructor() {
        this.value = null;
    }

    public function setValue(Object value):void {
        this.value = value;
        if (value == null) {
            print("Value set to null");
        } else {
            print("Value set to: " + value);
        }
    }

    public function getValue(): Object {
        return this.value;
    }

    public function hasValue(): bool {
        return this.value != null;
    }

    public function safeProcess(): void {
        if (this.value == null) {
            print("No value to process");
            return;
        }
        print("Processing value: " + this.value);
    }
}

class DataHolder extends Object {
    private string data;

    public constructor(string data) {
        this.data = data;
    }

    public function getData(): string {
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
Object? obj = null;
print("obj is null: " + (obj == null));
obj = new DataHolder("Data");
print("obj is not null: " + (obj != null));

print("\nTest 5: Null comparison");
Container c1 = new Container();
Container c2 = new Container();
c1.setValue(null);
c2.setValue(null);
print("Both null, same value: " + (c1.getValue() == c2.getValue()));
