// Test arrays of generic types with proper type checking
import * from "../../../lib/primitives/Int.mt";

class Box<T> {
    private T value;

    constructor(T val) {
        value = val;
    }

    T getValue(): T {
        return value;
    }

    void setValue(T val): void {
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

    K getKey(): K {
        return key;
    }

    V getValue(): V {
        return value;
    }
}

print("Testing arrays of generic types");

// Test array of Box<Int>
Box<Int>[] intBoxes = new Box<Int>[3];
intBoxes[0] = new Box<Int>(new Int(10));
intBoxes[1] = new Box<Int>(new Int(20));
intBoxes[2] = new Box<Int>(new Int(30));

print("IntBox[0]: " + intBoxes[0].getValue().getValue());
print("IntBox[2]: " + intBoxes[2].getValue().getValue());

// Test array of Box<string>
Box<string>[] stringBoxes = new Box<string>[2];
stringBoxes[0] = new Box<string>("Hello");
stringBoxes[1] = new Box<string>("World");

print("StringBox[0]: " + stringBoxes[0].getValue());
print("StringBox[1]: " + stringBoxes[1].getValue());

// Test array of Pair<string, Int>
Pair<string, Int>[] pairs = new Pair<string, Int>[2];
pairs[0] = new Pair<string, Int>("age", new Int(25));
pairs[1] = new Pair<string, Int>("score", new Int(100));

print("Pair[0] key: " + pairs[0].getKey());
print("Pair[0] value: " + pairs[0].getValue().getValue());
print("Pair[1] key: " + pairs[1].getKey());
print("Pair[1] value: " + pairs[1].getValue().getValue());

print("Generic array type checking passed");
