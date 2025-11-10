import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/Bool.mt";

// Test class: Box<T> - simple container
class Box<T> {
    T value;

    constructor(T val) {
        value = val;
    }

    public function getValue(): T {
        return value;
    }
}

// Test class: Pair<K, V> - two type parameters
class Pair<K, V> {
    K key;
    V val;

    constructor(K k, V v) {
        key = k;
        val = v;
    }

    public function getKey(): K {
        return key;
    }

    public function getValue(): V {
        return val;
    }
}

// PHASE 2 TEST: Nested generic inference - single type parameter
function <T> unbox(Box<T> container): T {
    return container.getValue();
}

// PHASE 2 TEST: Nested generic inference - multiple type parameters
function <K, V> extractValue(Pair<K, V> pair): V {
    return pair.getValue();
}

// PHASE 2 TEST: Nested generic inference - same type parameter in multiple positions
function <T> combineBoxes(Box<T> first, Box<T> second): T {
    // Just return first value for testing
    return first.getValue();
}

// Test 1: Simple nested inference - Box<Int>
Box<Int> intBox = new Box<Int>(new Int(42));
Int result1 = unbox(intBox);  // Should infer T=Int from Box<Int>
print("Unboxed Int: " + result1.toString());

// Test 2: Simple nested inference - Box<String>
Box<String> strBox = new Box<String>(new String("hello"));
String result2 = unbox(strBox);  // Should infer T=String from Box<String>
print("Unboxed String: " + result2.toString());

// Test 3: Multiple type parameters - Pair<String, Int>
Pair<String, Int> pair1 = new Pair<String, Int>(new String("age"), new Int(25));
Int result3 = extractValue(pair1);  // Should infer K=String, V=Int
print("Extracted Int from pair: " + result3.toString());

// Test 4: Multiple type parameters - Pair<Int, String>
Pair<Int, String> pair2 = new Pair<Int, String>(new Int(100), new String("score"));
String result4 = extractValue(pair2);  // Should infer K=Int, V=String
print("Extracted String from pair: " + result4);

// Test 5: Same type parameter in multiple positions
Box<Bool> box1 = new Box<Bool>(new Bool(true));
Box<Bool> box2 = new Box<Bool>(new Bool(false));
Bool result5 = combineBoxes(box1, box2);  // Should infer T=Bool from both boxes
print("Combined boxes: " + result5.toString());

print("All nested generic inference tests passed!");
