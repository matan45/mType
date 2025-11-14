import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/collections/ArrayList.mt";

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
// ArrayList<Box<T>> - two levels of nesting
function <T> processNestedBoxes(ArrayList<Box<T>> boxes): T {
    Box<T> firstBox = boxes.get(0);
    return firstBox.getValue();
}

// PHASE 2 TEST: Even deeper nesting - ArrayList<ArrayList<T>>
function <T> flattenFirst(ArrayList<ArrayList<T>> nested): T {
    ArrayList<T> innerArrayList = nested.get(0);
    return innerArrayList.get(0);
}

// Test 1: Two-level nesting - ArrayList<Box<Int>>
ArrayList<Box<Int>> boxedInts = new ArrayList<Box<Int>>();
boxedInts.add(new Box<Int>(new Int(100)));
boxedInts.add(new Box<Int>(new Int(200)));

Int result1 = processNestedBoxes(boxedInts);  // Should infer T=Int from ArrayList<Box<Int>>
print("Deep nested Int: " + result1.toString());

// Test 2: Two-level nesting - ArrayList<Box<String>>
ArrayList<Box<String>> boxedStrings = new ArrayList<Box<String>>();
boxedStrings.add(new Box<String>(new String("hello")));
boxedStrings.add(new Box<String>(new String("world")));

String result2 = processNestedBoxes(boxedStrings);  // Should infer T=String from ArrayList<Box<String>>
print("Deep nested String: " + result2);

// Test 3: ArrayList<ArrayList<Int>>
ArrayList<ArrayList<Int>> nestedInts = new ArrayList<ArrayList<Int>>();
ArrayList<Int> innerArrayList1 = new ArrayList<Int>();
innerArrayList1.add(new Int(10));
innerArrayList1.add(new Int(20));
nestedInts.add(innerArrayList1);

Int result3 = flattenFirst(nestedInts);  // Should infer T=Int from ArrayList<ArrayList<Int>>
print("Flattened nested Int: " + result3.toString());

print("Deep nested generic inference tests passed!");
