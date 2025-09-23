import "../../lib/primitives/Int.mt";
import "../../lib/primitives/String.mt";

// Generic class with multiple type parameters
class Pair<T, U> {
    T first;
    U second;

    function setFirst(T value): void {
        first = value;
    }

    function setSecond(U value): void {
        second = value;
    }

    function getFirst(): T {
        return first;
    }

    function getSecond(): U {
        return second;
    }
}

function main(): void {
    Pair<Int, String> pair = new Pair<Int, String>();
    pair.setFirst(new Int(100));
    pair.setSecond(new String("Hello"));

    Int firstValue = pair.getFirst();
    String secondValue = pair.getSecond();

    print("First: " + firstValue);
    print("Second: " + secondValue);
}

main();