import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Test generic class instantiation with proper type checking
class Pair<T, U> {
    T first;
    U second;

    constructor(T f, U s) {
        first = f;
        second = s;
    }

    public function getFirst(): T {
        return first;
    }

    public function getSecond(): U {
        return second;
    }

    public function setFirst(T newFirst): void {
        first = newFirst;
    }

    public function setSecond(U newSecond): void {
        second = newSecond;
    }
}

class Triple<T, U, V> {
    T first;
    U second;
    V third;

    constructor(T f, U s, V t) {
        first = f;
        second = s;
        third = t;
    }

    public function getAll(): string {
        return "" + first.toString() + ", " + second.toString() + ", " + third.toString();
    }
}

function main(): void {
    print("Testing generic class instantiation");

    // Test Pair<Int, String>
    Int num = new Int(42);
    String str = new String("Answer");
    Pair<Int, String> pair1 = new Pair<Int, String>(num, str);
    print("Pair first: " + pair1.getFirst());
    print("Pair second: " + pair1.getSecond());

    // Test Pair<String, Int>
    String str2 = new String("Count");
    Int num2 = new Int(100);
    Pair<String, Int> pair2 = new Pair<String, Int>(str2, num2);
    print("Pair2 first: " + pair2.getFirst());
    print("Pair2 second: " + pair2.getSecond());

    // Test setters with type checking
    pair1.setFirst(new Int(99));
    pair1.setSecond(new String("Updated"));
    print("Updated pair first: " + pair1.getFirst());
    print("Updated pair second: " + pair1.getSecond());

    // Test Triple
    Triple<Int, String, Int> triple = new Triple<Int, String, Int>(
        new Int(1),
        new String("middle"),
        new Int(3)
    );
    print("Triple: " + triple.getAll());

    print("Generic class instantiation test completed");
}

main();
