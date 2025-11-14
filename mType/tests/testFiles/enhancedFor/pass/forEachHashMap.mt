// Test: Enhanced for-loop with HashMap
// Tests for (K key : map) syntax

import * from "../../lib/collections/HashMap.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    HashMap<String, Int> map = new HashMap<String, Int>();
    map.put(new String("alpha"), new Int(100));
    map.put(new String("beta"), new Int(200));
    map.put(new String("gamma"), new Int(300));

    print("Enhanced for-loop over HashMap keys:");

    int count = 0;
    for (String key : map) {
        Int value = map.get(key);
        print(key + " -> " + value);
        count = count + 1;
    }

    print("Count: " + count);
    print("HashMap enhanced for-loop test passed!");
}

main();
