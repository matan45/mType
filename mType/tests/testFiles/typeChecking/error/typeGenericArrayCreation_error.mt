import * from "../../../lib/primitives/Int.mt";

// Test generic array creation restrictions
// Generic arrays should not be allowed due to type erasure issues
class Box<T> {
    T value;

    constructor(T v) {
        value = v;
    }

    public function getValue(): T {
        return value;
    }
}

function main(): void {
    // This should fail: cannot create generic array directly
    // Generic types are erased at runtime, making array creation unsafe
    Box<Int>[] boxes = new Box<Int>[5];  // ERROR: Generic array creation not allowed

    print("This should not execute");
}

main();
