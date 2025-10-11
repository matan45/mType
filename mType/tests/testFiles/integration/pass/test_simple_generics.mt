// Simple Generic Type Test
// Tests basic generic type handling without complex collections
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/Bool.mt";
import * from "../../lib/primitives/Float.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/collections/List.mt";

class GenericBox<T> {
    T value;

    public constructor(T val) {
        this.value = val;
    }

    public function getValue():T {
        return this.value;
    }

    public function setValue(T val):void {
        this.value = val;
    }

    public function toString(): string {
        return "Box(" + value.toString() + ")";
    }
}

class Pair<K, V> {
    K key;
    V value;

    public constructor(K k, V v) {
        this.key = k;
        this.value = v;
    }

    public function getKey(): K {
        return this.key;
    }

    public function getValue(): V {
        return this.value;
    }

    public function toString(): string {
        return "(" + key.toString() + ", " + value.toString() + ")";
    }
}

function testBasicGenerics():void {
    print("=== Testing Basic Generics ===");

    // Test generic box with different types
    GenericBox<Int> intBox = new GenericBox<Int>(new Int(42));
    print("Int box: " + intBox.toString());
    print("Int box value: " + intBox.getValue().toString());

    GenericBox<String> stringBox = new GenericBox<String>(new String("Hello Generics!"));
    print("String box: " + stringBox.toString());
    print("String box value: " + stringBox.getValue());

    GenericBox<Bool> boolBox = new GenericBox<Bool>(new Bool(true));
    print("Bool box: " + boolBox.toString());
    print("Bool box value: " + boolBox.getValue().toString());
}

function testGenericPairs():void {
    print("=== Testing Generic Pairs ===");

    // Test pairs with different key-value combinations
    Pair<String, Int> nameAge = new Pair<String, Int>(new String("Alice"),new Int(25));
    print("Name-Age pair: " + nameAge.toString());
    print("Name: " + nameAge.getKey());
    print("Age: " + nameAge.getValue().toString());

    Pair<Int, string> idName = new Pair<Int, String>(new Int(101), new String("Bob"));
    print("ID-Name pair: " + idName.toString());
    print("ID: " + idName.getKey().toString());
    print("Name: " + idName.getValue());

    Pair<Bool, Float> statusScore = new Pair<Bool, Float>(new Bool(true),new Float(95.5));
    print("Status-Score pair: " + statusScore.toString());
    print("Status: " + statusScore.getKey().toString());
    print("Score: " + statusScore.getValue().toString());
}

class Student {
        string name;
        int grade;

        public constructor(string n, int g) {
            this.name = n;
            this.grade = g;
        }

        public function toString(): string {
            return name + " (Grade: " + grade + ")";
        }
    }

function testGenericWithCustomObjects():void {
    print("=== Testing Generics with Custom Objects ===");

    // Test generic box with custom object
    Student alice = new Student("Alice", 90);
    GenericBox<Student> studentBox = new GenericBox<Student>(alice);
    print("Student box: " + studentBox.toString());

    Student retrieved = studentBox.getValue();
    print("Retrieved student: " + retrieved.toString());

    // Test pair with custom objects
    Student bob = new Student("Bob", 85);
    Pair<Student, Int> studentScore = new Pair<Student, Int>(bob, new Int(1000));
    print("Student-Score pair: " + studentScore.toString());
}

function testArraysWithGenerics():void {
    print("=== Testing Arrays with Generics ===");

    // Test array of generic boxes
    List<GenericBox<Int>> boxes = new List<GenericBox<Int>>();
    boxes.add(new GenericBox<Int>(new Int(10)));
    boxes.add(new GenericBox<Int>(new Int(20)));
    boxes.add(new GenericBox<Int>(new Int(30)));

    print("Array of generic boxes:");
    for (int i = 0; i < boxes.size(); i++) {
        print("  [" + i + "] = " + boxes.get(i).toString());
    }

    // Test array of pairs
    List<Pair<String, Int>> pairs = new List<Pair<String, Int>>();
    pairs.add(new Pair<String, Int>(new String("First"), new Int(1)));
    pairs.add(new Pair<String, Int>(new String("Second"), new Int(2)));

    print("Array of pairs:");
    for (int i = 0; i < pairs.size(); i++) {
        print("  [" + i + "] = " + pairs.get(i).toString());
    }
}

function main():void {
    print("Starting Simple Generic Type Tests...");

    testBasicGenerics();
    testGenericPairs();
    testGenericWithCustomObjects();
    testArraysWithGenerics();

    print("Simple generic type tests completed!");
}

main();