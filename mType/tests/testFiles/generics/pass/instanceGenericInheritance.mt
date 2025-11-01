import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";

// Test: Inheritance with generic instance methods
class Base {
    public function <T> process(T value): T {
        print("Base processing: " + value);
        return value;
    }
}

class Derived extends Base {
    // Override generic instance method
    public function <T> process(T value): T {
        print("Derived processing: " + value);
        return value;
    }
}

function main(): void {
    // Test base class
    Base base = new Base();
    String baseResult = base.process<String>(new String("base"));
    print("Base result: " + baseResult);

    // Test derived class
    Derived derived = new Derived();
    String derivedResult = derived.process<String>(new String("derived"));
    print("Derived result: " + derivedResult);

    // Test polymorphism - derived object as base type
    Base polyBase = new Derived();
    Int polyResult = polyBase.process<Int>(new Int(42));
    print("Polymorphic result: " + polyResult);

    print("Generic method inheritance test passed");
}

main();
