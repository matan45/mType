// Test: Malformed generic - multiple consecutive commas
// Expected: Should fail at parse time with clear error message

import * from "../../lib/collections/HashMap.mt";

class Container {
    // ERROR: Multiple consecutive commas in type arguments
    HashMap<string,, int> data;

    constructor() {
        this.data = new HashMap<string, int>();
    }
}

function main(): void {
    Container c = new Container();
    print("This should not execute");
}

main();
