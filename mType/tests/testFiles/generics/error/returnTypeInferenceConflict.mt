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

// Function with type parameter in both argument and return type
function <T> wrapValue(T value): Box<T> {
    return new Box<T>(value);
}

function main(): void {
    // ERROR: Conflicting inference
    // Argument suggests T=Int
    // Expected return type suggests T=String
    Box<String> result = wrapValue(new Int(42));
}

main();
