// Test: Malformed generic - unmatched nested brackets
// Expected: Should fail at parse time with clear error message

import * from "../../lib/collections/HashMap.mt";
import * from "../../lib/collections/List.mt";

class DataStore {
    // ERROR: Unmatched nested generic brackets - missing > for List<int
    HashMap<string, List<int> data;

    constructor() {
        this.data = new HashMap<string, List<int>>();
    }
}

function main(): void {
    DataStore store = new DataStore();
    print("This should not execute");
}

main();
