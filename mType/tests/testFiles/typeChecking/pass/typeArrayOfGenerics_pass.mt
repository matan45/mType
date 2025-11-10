// Test arrays of generic types with proper type checking
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

class Box<T> {
    private T value;

    constructor(T val) {
        value = val;
    }

    public function getValue(): T {
        return value;
    }

    public function setValue(T val): void {
        value = val;
    }
}

class Pair<K, V> {
    private K key;
    private V value;

    constructor(K k, V v) {
        key = k;
        value = v;
    }

    public function getKey(): K {
        return key;
    }

    public function getValue(): V {
        return value;
    }
}

print("Testing arrays of generic types");

// Test array of Box<Int>
Box<Int>[] intBoxes = new Box<Int>[3];
intBoxes[0] = new Box<Int>(10);
intBoxes[1] = new Box<Int>(20);
intBoxes[2] = new Box<Int>(30);

print("IntBox[0]: " + intBoxes[0].getValue().getValue());
print("IntBox[2]: " + intBoxes[2].getValue().getValue());

// Test array of Box<string>
Box<String>[] stringBoxes = new Box<String>[2];
stringBoxes[0] = new Box<String>("Hello");
stringBoxes[1] = new Box<String>("World");

print("StringBox[0]: " + stringBoxes[0].getValue());
print("StringBox[1]: " + stringBoxes[1].getValue());

// Test array of Pair<string, Int>
Pair<String, Int>[] pairs = new Pair<String, Int>[2];
pairs[0] = new Pair<String, Int>("age", 25);
pairs[1] = new Pair<String, Int>("score", 100);

print("Pair[0] key: " + pairs[0].getKey().toString());
print("Pair[0] value: " + pairs[0].getValue().getValue());
print("Pair[1] key: " + pairs[1].getKey().toString());
print("Pair[1] value: " + pairs[1].getValue().getValue());

print("Generic array type checking passed");
