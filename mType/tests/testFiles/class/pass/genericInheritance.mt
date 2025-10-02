// Test: Generic inheritance - Generic parent classes
// Expected: Pass - demonstrates inheritance with generic types

class Container<T> {
    T value;

    constructor(T val) {
        this.value = val;
    }

    function getValue(): T {
        return this.value;
    }

    function setValue(T val): void {
        this.value = val;
    }
}

class Box<T> extends Container<T> {
    String label;

    constructor(T val, String label) {
        super(val);
        this.label = label;
    }

    function getLabel(): String {
        return this.label;
    }

    function describe(): String {
        return "Box '" + this.label + "' contains: " + this.value;
    }
}

// Test with String type
Box<String> stringBox = new Box<String>("Hello World", "Messages");
print(stringBox.describe());
print("Value: " + stringBox.getValue());
print("Label: " + stringBox.getLabel());

// Test with int type
Box<int> intBox = new Box<int>(42, "Numbers");
print(intBox.describe());
print("Value: " + intBox.getValue());
print("Label: " + intBox.getLabel());

// Test setValue from parent
stringBox.setValue("New Message");
print("Updated: " + stringBox.getValue());
