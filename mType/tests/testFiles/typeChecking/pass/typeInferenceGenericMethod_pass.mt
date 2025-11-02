import * from "../../../lib/primitives/Int.mt";
import * from "../../../lib/primitives/String.mt";
import * from "../../../lib/primitives/Bool.mt";

// Test generic method type argument inference from parameters

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
}

// Generic method that creates a pair - type should be inferred from arguments
function <T, U> makePair(T first, U second): Pair<T, U> {
    return new Pair<T, U>(first, second);
}

// Generic method that returns the first of two items
function <T> first(T a, T b): T {
    return a;
}

// Generic method that swaps pair elements
function <T, U> swap(Pair<T, U> pair): Pair<U, T> {
    return new Pair<U, T>(pair.getSecond(), pair.getFirst());
}

// Generic method with bound type parameter
function <T> max(T a, T b, Bool useFirst): T {
    if (useFirst) {
        return a;
    }
    return b;
}

function main(): void {
    print("Testing generic method type argument inference");

    // Test makePair with Int and String - types inferred from arguments
    Pair<Int, String> p1 = makePair<Int, String>(new Int(42), new String("answer"));
    print("Pair<Int, String>: (" + p1.getFirst() + ", " + p1.getSecond() + ")");

    // Test first with String
    String s1 = new String("Hello");
    String s2 = new String("World");
    String result1 = first<String>(s1, s2);
    print("first<String>(Hello, World): " + result1);

    // Test swap
    Pair<String, Int> p2 = swap<Int, String>(p1);
    print("swapped pair: (" + p2.getFirst() + ", " + p2.getSecond() + ")");

    // Test max with Int
    Int a = new Int(100);
    Int b = new Int(200);
    Int maxVal = max<Int>(a, b, new Bool(false));
    print("max<Int>(100, 200, false): " + maxVal);

    // Test nested generic inference
    Pair<String, Bool> p3 = makePair<String, Bool>(new String("test"), new Bool(true));
    print("Pair<String, Bool>: (" + p3.getFirst() + ", " + p3.getSecond() + ")");

    print("Generic method inference test completed");
}

main();
