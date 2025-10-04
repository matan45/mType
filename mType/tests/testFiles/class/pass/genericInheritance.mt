// Test: Generic inheritance - Generic parent classes
// Expected: Pass - demonstrates inheritance with generic types
import "../../lib/primitives/String.mt";
import "../../lib/primitives/Int.mt";

class Container<T> {
    public T value;

    public constructor(T val) {
        this.value = val;
    }

    public function getValue(): T {
        return this.value;
    }

    public function setValue(T val): void {
        this.value = val;
    }
}

class Box<T> extends Container<T> {
    public String label;

    public constructor(T val, String label): super(val) {
        this.label = label;
    }

    public function getLabel(): String {
        return this.label;
    }

    public function describe(): String {
        return new String("Box '" + this.label + "' contains: " + this.value);
    }
}

// Test with String type
Box<String> stringBox = new Box<String>(new String("Hello World"), new String("Messages"));
print(stringBox.describe());
print("Value: " + stringBox.getValue());
print("Label: " + stringBox.getLabel());

// Test with int type
Box<Int> intBox = new Box<Int>(new Int(42), new String("Numbers"));
print(intBox.describe());
print("Value: " + intBox.getValue());
print("Label: " + intBox.getLabel());

// Test setValue from parent
stringBox.setValue(new String("New Message"));
print("Updated: " + stringBox.getValue());
