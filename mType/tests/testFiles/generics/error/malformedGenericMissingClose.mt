// Test: Malformed generic - missing closing bracket
// Expected: Should fail at parse time with clear error message

import * from "../../lib/collections/List.mt";

class Container {
    // ERROR: Missing closing bracket on generic type
    List<string items;

    constructor() {
        this.items = new List<string>();
    }
}

function main(): void {
    Container c = new Container();
    print("This should not execute");
}

main();
