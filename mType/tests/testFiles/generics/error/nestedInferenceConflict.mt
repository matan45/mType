import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";

// Test class: Box<T>
class Box<T> {
    T value;

    constructor(T val) {
        value = val;
    }

    public function getValue(): T {
        return value;
    }
}

// Function with same type parameter in multiple positions
function <T> combineBoxes(Box<T> first, Box<T> second): T {
    return first.getValue();
}

function main(): void {
    Box<Int> intBox = new Box<Int>(new Int(42));
    Box<String> strBox = new Box<String>(new String("hello"));

    // ERROR: T cannot be both Int and String
    // First parameter suggests T=Int, second suggests T=String
    combineBoxes(intBox, strBox);
}

main();
