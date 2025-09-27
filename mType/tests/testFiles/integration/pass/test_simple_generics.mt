// Simple Generic Type Test
// Tests basic generic type handling without complex collections
import "../../lib/primitives/Int.mt";
import "../../lib/primitives/Bool.mt";
import "../../lib/primitives/Float.mt";
import "../../lib/primitives/String.mt";

class GenericBox<T> {
    T value;

    constructor(T val) {
        this.value = val;
    }

    function getValue():T {
        return this.value;
    }

    function setValue(T val):void {
        this.value = val;
    }

    function toString(): string {
        return "Box(" + value.toString() + ")";
    }
}

class Pair<K, V> {
    K key;
    V value;

    constructor(K k, V v) {
        this.key = k;
        this.value = v;
    }

    function getKey(): K {
        return this.key;
    }

    function getValue(): V {
        return this.value;
    }

    function toString(): string {
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

function testGenericWithCustomObjects():void {
    print("=== Testing Generics with Custom Objects ===");

    class Student {
        string name;
        int grade;

        constructor(string n, int g) {
            this.name = n;
            this.grade = g;
        }

        function toString(): string {
            return name + " (Grade: " + grade + ")";
        }
    }

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
    GenericBox<Int>[] boxes = new GenericBox<Int>[3];
    boxes[0] = new GenericBox<Int>(new Int(10));
    boxes[1] = new GenericBox<int>(new Int(20));
    boxes[2] = new GenericBox<int>(new Int(30));

    print("Array of generic boxes:");
    for (int i = 0; i < boxes.length; i++) {
        print("  [" + i + "] = " + boxes[i].toString());
    }

    // Test array of pairs
    Pair<String, Int>[] pairs = new Pair<String, Int>[2];
    pairs[0] = new Pair<string, int>(new String("First"), new Int(1));
    pairs[1] = new Pair<string, int>(new String("Second"), new Int(2));

    print("Array of pairs:");
    for (int i = 0; i < pairs.length; i++) {
        print("  [" + i + "] = " + pairs[i].toString());
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