import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/Float.mt";

// Test independence of class-level type parameter and method-level type parameter.
// mType rejects re-using the same letter (treated as shadowing), so we use
// a distinct letter <U> on the method. The intent is the same: the method's
// type parameter is independent from the class's type parameter.
class Box<T> {
    T value;

    public constructor(T initial) {
        this.value = initial;
    }

    public function getValue(): T {
        return this.value;
    }

    // Method-level <U> is independent from class-level <T>.
    // The method takes a U and returns a U, independent of T.
    public function <U> foo(U x): U {
        print("foo called with: " + x);
        return x;
    }
}

function main(): void {
    print("Testing generic parameter independence");

    // Class T = Int, method U = String -> independent.
    Box<Int> intBox = new Box<Int>(new Int(42));
    print("Class T value: " + intBox.getValue().getValue());

    String s = intBox.foo<String>(new String("hello"));
    print("Method U returned: " + s);

    // Class T = String, method U = Float -> independent.
    Box<String> strBox = new Box<String>(new String("world"));
    print("Class T value: " + strBox.getValue().getValue());

    Float f = strBox.foo<Float>(new Float(3.5));
    print("Method U returned: " + f.getValue());

    print("Generic parameter independence test completed");
}

main();
