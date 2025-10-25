// Test: Malformed generic - numeric literal as type argument
// Expected: Should fail - numeric literals not allowed as type arguments

import * from "../../lib/collections/List.mt";

class Container {
    // ERROR: Numeric literal (123) not allowed as type argument
    List<123> items;

    constructor() {
        this.items = new List<123>();
    }
}

function main(): void {
    Container c = new Container();
    print("This should not execute");
}

main();
