import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/collections/List.mt";

// Test class: Box<T>
class Box<T> {
    T value;

    constructor(T val) {
        value = val;
    }

    public function getValue(): T {
        return value;
    }
}

// PHASE 2 TEST: Multi-level nested generic inference
// List<Box<T>> - two levels of nesting
function <T> processNestedBoxes(List<Box<T>> boxes): T {
    Box<T> firstBox = boxes.get(0);
    return firstBox.getValue();
}

// PHASE 2 TEST: Even deeper nesting - List<List<T>>
function <T> flattenFirst(List<List<T>> nested): T {
    List<T> innerList = nested.get(0);
    return innerList.get(0);
}

// Test 1: Two-level nesting - List<Box<Int>>
List<Box<Int>> boxedInts = new List<Box<Int>>();
boxedInts.add(new Box<Int>(new Int(100)));
boxedInts.add(new Box<Int>(new Int(200)));

Int result1 = processNestedBoxes(boxedInts);  // Should infer T=Int from List<Box<Int>>
print("Deep nested Int: " + result1.toString());

// Test 2: Two-level nesting - List<Box<String>>
List<Box<String>> boxedStrings = new List<Box<String>>();
boxedStrings.add(new Box<String>(new String("hello")));
boxedStrings.add(new Box<String>(new String("world")));

String result2 = processNestedBoxes(boxedStrings);  // Should infer T=String from List<Box<String>>
print("Deep nested String: " + result2);

// Test 3: List<List<Int>>
List<List<Int>> nestedInts = new List<List<Int>>();
List<Int> innerList1 = new List<Int>();
innerList1.add(new Int(10));
innerList1.add(new Int(20));
nestedInts.add(innerList1);

Int result3 = flattenFirst(nestedInts);  // Should infer T=Int from List<List<Int>>
print("Flattened nested Int: " + result3.toString());

print("Deep nested generic inference tests passed!");
