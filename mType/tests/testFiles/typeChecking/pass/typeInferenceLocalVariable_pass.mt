import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Float.mt";
import * from "../../lib/primitives/Bool.mt";

// Test local variable type inference

class Box<T> {
    T value;

    constructor(T val) {
        value = val;
    }

    public function getValue(): T {
        return value;
    }
}

function main(): void {
    print("Testing local variable type inference");

    // Test integer inference
    Int x = new Int(42);
    print("Int value: " + x);

    // Test string inference
    String name = new String("mType");
    print("String value: " + name);

    // Test float inference
    Float pi = new Float(3.14);
    print("Float value: " + pi);

    // Test boolean inference
    Bool flag = new Bool(true);
    print("Bool value: " + flag);

    // Test object inference
    Box<Int> box = new Box<Int>(new Int(100));
    print("Box<Int> value: " + box.getValue());

    // Test nested generic inference
    Box<String> strBox = new Box<String>(new String("Inference"));
    print("Box<String> value: " + strBox.getValue());

    // Test reassignment with same type
    x = new Int(99);
    print("Reassigned Int: " + x);

    print("Local variable inference test completed");
}

main();
