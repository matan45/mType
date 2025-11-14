// Test: HashMap entry iterator
// Tests entryIterator() method on HashMap

import * from "../../lib/collections/HashMap.mt";
import * from "../../lib/interfaces/MapEntry.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/Iterator.mt";

function main(): void {
    HashMap<String, Int> map = new HashMap<String, Int>();
    map.put(new String("apple"), new Int(1));
    map.put(new String("banana"), new Int(2));
    map.put(new String("cherry"), new Int(3));

    print("Testing HashMap entry iterator:");

    Iterator<MapEntry<String, Int>> iter = map.entryIterator();
    int count = 0;

    while (iter.hasNext()) {
        MapEntry<String, Int> entry = iter.next();
        String key = entry.getKey();
        Int value = entry.getValue();
        print(key + " = " + value);
        count = count + 1;
    }

    iter.close();

    print("Count: " + count);
    print("HashMap entry iterator test passed!");
}

main();
