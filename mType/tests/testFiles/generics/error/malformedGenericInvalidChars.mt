// Test: Malformed generic - invalid characters in type arguments
// Expected: Should fail at parse time with clear error message

import * from "../../lib/collections/List.mt";

class Container {
    // ERROR: Invalid characters (@, #) in generic type argument
    List<string@#> items;

    constructor() {
        this.items = new List<string>();
    }
}

function main(): void {
    Container c = new Container();
    print("This should not execute");
}

main();
