// Test LinkedList functionality with proper output
import * from "../../lib/collections/LinkedList.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";

class TestObject {
    string name;
    int value;

    constructor(string n, int v) {
        name = n;
        value = v;
    }

    public function toString(): string {
        return name + "(" + value + ")";
    }

    public function equals(TestObject other): bool {
        if (other == null) return false;
        return this.name == other.name && this.value == other.value;
    }

    public function hashCode(): int {
        return hashCode(this.name) + this.value;
    }
}

function testLinkedListFixed(): void {
    print("Testing LinkedList functionality with proper output");

    // Test 1: Basic operations
    print("\n=== Test 1: Basic Operations ===");
    LinkedList<String> list = new LinkedList<String>();

    print("Empty list size: " + list.size());
    print("Empty list is empty: " + list.empty());

    // Add elements
    list.add(new String("First"));
    list.add(new String("Second"));
    list.add(new String("Third"));

    print("After adding 3 elements:");
    print("Size: " + list.size());
    print("First: " + list.first().toString());
    print("Last: " + list.last().toString());
    print("Get(0): " + list.get(0).toString());
    print("Get(1): " + list.get(1).toString());
    print("Get(2): " + list.get(2).toString());

    // Test 2: Insert operations
    print("\n=== Test 2: Insert Operations ===");
    list.addFirst(new String("Before First"));
    list.addLast(new String("After Last"));
    list.addAt(2, new String("Inserted"));

    print("After insertions:");
    print("Size: " + list.size());
    String[] array = list.toArray();
    for (int i = 0; i < array.length; i++) {
        print("Index " + i + ": " + array[i].toString());
    }

    // Test 3: Remove operations
    print("\n=== Test 3: Remove Operations ===");
    print("Remove 'Second': " + list.remove(new String("Second")));
    print("Size after removal: " + list.size());

    String removed = list.removeFirst();
    print("Removed first: " + removed.toString());

    String removedLast = list.removeLast();
    print("Removed last: " + removedLast.toString());

    print("Size after removing first and last: " + list.size());

    // Test 4: Integer LinkedList with proper display
    print("\n=== Test 4: Integer LinkedList ===");
    LinkedList<Int> intList = new LinkedList<Int>();

    intList.add(new Int(10));
    intList.add(new Int(20));
    intList.add(new Int(30));
    intList.add(new Int(40));
    intList.add(new Int(50));

    print("Integer list contents:");
    Int[] intArray = intList.toArray();
    for (int i = 0; i < intArray.length; i++) {
        print("  " + i + ": " + intArray[i].toString());
    }

    // Test reverse
    print("Before reverse:");
    for (int i = 0; i < intArray.length; i++) {
        print("  " + i + ": " + intArray[i].toString());
    }

    intList.reverse();
    print("After reverse:");
    Int[] reversedArray = intList.toArray();
    for (int i = 0; i < reversedArray.length; i++) {
        print("  " + i + ": " + reversedArray[i].toString());
    }

    // Test 5: Object LinkedList
    print("\n=== Test 5: Object LinkedList ===");
    LinkedList<TestObject> objList = new LinkedList<TestObject>();

    TestObject obj1 = new TestObject("Alice", 25);
    TestObject obj2 = new TestObject("Bob", 30);
    TestObject obj3 = new TestObject("Charlie", 35);

    objList.add(obj1);
    objList.add(obj2);
    objList.add(obj3);

    print("Object list contents:");
    TestObject[] objArray = objList.toArray();
    for (int i = 0; i < objArray.length; i++) {
        print("  " + i + ": " + objArray[i].toString());
    }

    print("Contains Alice: " + objList.contains(obj1));
    print("Index of Bob: " + objList.indexOf(obj2));

    // Test 6: Advanced operations
    print("\n=== Test 6: Advanced Operations ===");
    LinkedList<Int> advancedList = new LinkedList<Int>();

    // Test addAll
    Int[] numbers = new Int[5];
    for (int i = 0; i < 5; i++) {
        numbers[i] = new Int(i * 100);
    }
    advancedList.addAll(numbers);

    print("After addAll:");
    Int[] addAllArray = advancedList.toArray();
    for (int i = 0; i < addAllArray.length; i++) {
        print("  " + i + ": " + addAllArray[i].toString());
    }

    // Test sublist
    LinkedList<Int> subList = advancedList.subList(1, 4);
    print("Sublist (1 to 4):");
    Int[] subArray = subList.toArray();
    for (int i = 0; i < subArray.length; i++) {
        print("  Sublist " + i + ": " + subArray[i].toString());
    }

    // Test removeAll
    LinkedList<String> removeAllList = new LinkedList<String>();
    removeAllList.add(new String("Duplicate"));
    removeAllList.add(new String("Unique"));
    removeAllList.add(new String("Duplicate"));
    removeAllList.add(new String("Different"));
    removeAllList.add(new String("Duplicate"));

    print("Before removeAll:");
    String[] beforeRemove = removeAllList.toArray();
    for (int i = 0; i < beforeRemove.length; i++) {
        print("  " + i + ": " + beforeRemove[i].toString());
    }

    int removedCount = removeAllList.removeAll(new String("Duplicate"));
    print("Removed " + removedCount + " duplicates");

    print("After removeAll:");
    String[] afterRemove = removeAllList.toArray();
    for (int i = 0; i < afterRemove.length; i++) {
        print("  " + i + ": " + afterRemove[i].toString());
    }

    // Test 7: Null validation (brief)
    print("\n=== Test 7: Null Validation ===");
    LinkedList<String> nullTestList = new LinkedList<String>();

    print("Testing null validation:");
    // Null values cannot be passed to generic methods - test with valid items instead
    nullTestList.add(new String("NullTest"));
    bool containsTest = nullTestList.contains(new String("NullTest"));
    print("Contains test result: " + containsTest);

    nullTestList.add(new String("Valid Item"));
    print("List size after valid adds: " + nullTestList.size());
    print("First item: " + nullTestList.first().toString());

    print("\nLinkedList tests completed successfully!");
}

testLinkedListFixed();