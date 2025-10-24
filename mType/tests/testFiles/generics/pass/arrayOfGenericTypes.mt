import * from "../../lib/collections/List.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Test arrays containing generic types
class Box<T> {
    T value;

    public function setValue(T newValue): void {
        value = newValue;
    }

    public function getValue(): T {
        return value;
    }
}

function main(): void {
    print("Testing arrays of generic types");

    // Test 1: Array of generic Box<Int>
    Box<Int>[] intBoxes = new Box<Int>[3];
    intBoxes[0] = new Box<Int>();
    intBoxes[0].setValue(new Int(10));
    intBoxes[1] = new Box<Int>();
    intBoxes[1].setValue(new Int(20));
    intBoxes[2] = new Box<Int>();
    intBoxes[2].setValue(new Int(30));

    print("Box array created with 3 elements");
    print("intBoxes[0] value: " + intBoxes[0].getValue());
    print("intBoxes[1] value: " + intBoxes[1].getValue());
    print("intBoxes[2] value: " + intBoxes[2].getValue());

    // Test 2: Array of generic List<String>
    List<String>[] stringLists = new List<String>[2];
    stringLists[0] = new List<String>();
    stringLists[0].add(new String("Hello"));
    stringLists[0].add(new String("World"));

    stringLists[1] = new List<String>();
    stringLists[1].add(new String("Foo"));
    stringLists[1].add(new String("Bar"));

    print("List array created with 2 elements");
    print("stringLists[0] size: " + stringLists[0].size());
    print("stringLists[1] size: " + stringLists[1].size());
    print("stringLists[0][0]: " + stringLists[0].get(0));
    print("stringLists[1][0]: " + stringLists[1].get(0));

    // Test 3: Nested generic arrays (Box<Box<Int>>)
    Box<Box<Int>> nestedBox = new Box<Box<Int>>();
    Box<Int> innerBox = new Box<Int>();
    innerBox.setValue(new Int(42));
    nestedBox.setValue(innerBox);

    print("Nested box value: " + nestedBox.getValue().getValue());

    print("Array of generic types test completed");
}

main();
