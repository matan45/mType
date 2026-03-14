import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Bool.mt";

// Test conditional expression type inference

class Container<T> {
    T value;

    constructor(T val) {
        value = val;
    }

    public function getValue(): T {
        return value;
    }

    public function toString(): string {
        return "Container(" + value + ")";
    }
}

// Function that returns different values based on condition
function selectInt(Bool condition, Int a, Int b): Int {
    if (condition.getValue()) {
        return a;
    }
    return b;
}

function selectString(Bool condition, String a, String b): String {
    if (condition.getValue()) {
        return a;
    }
    return b;
}

// Function with multiple conditional branches
function classify(Int value): String {
    if (value.getValue() < 0) {
        return new String("negative");
    }
    if (value.getValue() == 0) {
        return new String("zero");
    }
    return new String("positive");
}

// Function with nested conditionals
function describe(Int x, Int y): String {
    if (x.getValue() > y.getValue()) {
        if (x.getValue() > 10) {
            return new String("x is large and greater than y");
        }
        return new String("x is greater than y");
    }
    if (x.getValue() == y.getValue()) {
        return new String("x equals y");
    }
    return new String("x is less than y");
}

function main(): void {
    print("Testing conditional expression type inference");

    // Test simple conditional with Int
    Int result1 = selectInt(new Bool(true), new Int(10), new Int(20));
    print("selectInt(true, 10, 20): " + result1);

    Int result2 = selectInt(new Bool(false), new Int(10), new Int(20));
    print("selectInt(false, 10, 20): " + result2);

    // Test conditional with String
    String result3 = selectString(new Bool(true), new String("yes"), new String("no"));
    print("selectString(true, yes, no): " + result3);

    String result4 = selectString(new Bool(false), new String("yes"), new String("no"));
    print("selectString(false, yes, no): " + result4);

    // Test multi-branch conditional
    String class1 = classify(new Int(-5));
    print("classify(-5): " + class1);

    String class2 = classify(new Int(0));
    print("classify(0): " + class2);

    String class3 = classify(new Int(7));
    print("classify(7): " + class3);

    // Test nested conditionals
    String desc1 = describe(new Int(15), new Int(5));
    print("describe(15, 5): " + desc1);

    String desc2 = describe(new Int(8), new Int(5));
    print("describe(8, 5): " + desc2);

    String desc3 = describe(new Int(5), new Int(5));
    print("describe(5, 5): " + desc3);

    String desc4 = describe(new Int(3), new Int(5));
    print("describe(3, 5): " + desc4);

    // Test conditional with generic type
    Container<Int> c1 = new Container<Int>(new Int(100));
    Container<Int> c2 = new Container<Int>(new Int(200));
    Container<Int> selected = selectInt(new Bool(true), new Int(1), new Int(2)).getValue() == 1 ? c1 : c2;
    print("Selected container: " + selected.toString());

    print("Conditional expression type inference test completed");
}

main();
