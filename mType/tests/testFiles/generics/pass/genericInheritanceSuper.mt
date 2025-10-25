// Test: Generic inheritance with super member access
// Expected: Should compile and run successfully
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

class Parent<T> {
    protected T value;

    constructor(T val) {
        this.value = val;
    }

    public function getValue(): T {
        return this.value;
    }

    public function display(): string {
        return "Parent: " + this.value.toString();
    }

    public function process(T item): T {
        return item;
    }
}

class Child<T> extends Parent<T> {
    private string childData;

    constructor(T val, string data): super(val) {
        this.childData = data;
    }

    public function display(): string {
        // Access super's method in generic context
        return super.display() + " | Child: " + this.childData;
    }

    public function getSuperValue(): T {
        // Access super's field
        return super.value;
    }

    public function testSuperProcess(T item): T {
        // Call super's generic method
        return super.process(item);
    }

    public function modifySuperValue(T newVal): void {
        super.value = newVal;
    }
}

// Test with different type parameters
class GrandChild<T, U> extends Child<T> {
    private U extraData;

    constructor(T val, string data, U extra): super(val, data) {
        this.extraData = extra;
    }

    public function displayAll(): string {
        return super.display() + " | GrandChild: " + this.extraData.toString();
    }

    public function getGrandSuperValue(): T {
        return super.getSuperValue();
    }
}

function main(): void {
    print("Testing generic inheritance with super access");

    // Test with String type
    Child<String> stringChild = new Child<String>(new String("Hello"), "World");
    print(stringChild.display());
    print("Super value: " + stringChild.getSuperValue().toString());
    print("Super process: " + stringChild.testSuperProcess(new String("Test")).toString());

    stringChild.modifySuperValue(new String("Modified"));
    print("After modification: " + stringChild.display());

    // Test with int type
    Child<Int> intChild = new Child<Int>(new Int(42), "Numbers");
    print(intChild.display());
    print("Super value: " + intChild.getSuperValue().toString());
    print("Super process: " + intChild.testSuperProcess(new Int(99)).toString());

    // Test with grandchild
    GrandChild<String, Int> grandChild = new GrandChild<String, Int>(new String("Base"), "Middle", new Int(123));
    print(grandChild.displayAll());
    print("GrandChild super value: " + grandChild.getGrandSuperValue().toString());

    // Test accessing parent's generic method through grandchild
    print("GrandChild process: " + grandChild.testSuperProcess(new String("GrandTest")).toString());

    print("Generic inheritance with super test passed!");
}

main();
