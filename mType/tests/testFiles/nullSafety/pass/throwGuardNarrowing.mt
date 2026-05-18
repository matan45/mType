// Test: guard clause with throw narrows the variable for subsequent code.
// ReturnPathValidator::pathAlwaysReturns treats ThrowNode as an exit path.

import * from "../../lib/exceptions/Exception.mt";

class Box {
    public int value;
    constructor(int v) {
        this.value = v;
    }
    public function getValue(): int {
        return this.value;
    }
}

function requireBox(Box? b): int {
    if (b == null) {
        throw new Exception("box was null");
    }
    return b.getValue();
}

function main(): void {
    print("good: " + requireBox(new Box(42)));
    try {
        requireBox(null);
        print("should not reach here");
    } catch (Exception e) {
        print("caught: " + e.getMessage());
    }
    print("Throw guard narrowing tests passed!");
}

main();
