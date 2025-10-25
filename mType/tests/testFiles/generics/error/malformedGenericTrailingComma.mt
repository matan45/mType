// Test: Malformed generic - trailing comma in type arguments
// Expected: Should fail at parse time with clear error message

import * from "../../lib/collections/HashMap.mt";

class Container {
    // ERROR: Trailing comma in type arguments
    HashMap<string, int,> data;

    constructor() {
        this.data = new HashMap<string, int>();
    }
}

function main(): void {
    Container c = new Container();
    print("This should not execute");
}

main();
