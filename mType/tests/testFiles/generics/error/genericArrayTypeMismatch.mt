import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Test generic array type mismatch
class Box<T> {
    T value;
}

function main(): void {
    // Create array of Box<Int>
    Box<Int>[] intBoxes = new Box<Int>[2];

    // ERROR: Cannot assign Box<String> to Box<Int> array
    intBoxes[0] = new Box<String>();

    print("This should not execute");
}

main();
