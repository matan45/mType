import * from "../../lib/primitives/Int.mt";

// Test Box<Box<T>> casting scenarios
class Value {
    int data;

    public function Value(int value) {
        data = value;
    }

    public function getValue(): int {
        return data;
    }
}

class Box<T> {
    T content;

    public function setContent(T item): void {
        content = item;
    }

    public function getContent(): T {
        return content;
    }
}

function main(): void {
    // Create nested box: Box<Box<Value>>
    Value value = new Value(42);
    Box<Value> innerBox = new Box<Value>();
    innerBox.setContent(value);

    Box<Box<Value>> outerBox = new Box<Box<Value>>();
    outerBox.setContent(innerBox);

    // Access nested content
    Box<Value> retrievedInner = outerBox.getContent();
    Value retrievedValue = retrievedInner.getContent();
    print("Nested value: " + retrievedValue.getValue());

    // Test with Int directly
    Box<Int> intBox = new Box<Int>();
    intBox.setContent(new Int(100));

    Box<Box<Int>> doubleIntBox = new Box<Box<Int>>();
    doubleIntBox.setContent(intBox);

    Box<Int> retrievedIntBox = doubleIntBox.getContent();
    Int retrievedInt = retrievedIntBox.getContent();
    print("Nested int: " + retrievedInt);

    print("Double nested generic cast successful");
}

main();
