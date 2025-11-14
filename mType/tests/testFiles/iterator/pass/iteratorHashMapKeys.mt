// Test: HashMap key iterator
// Tests iterator() method on HashMap

import * from "../../lib/collections/HashMap.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/Iterator.mt";

function main(): void {
    HashMap<String, Int> map = new HashMap<String, Int>();
    map.put(new String("one"), new Int(1));
    map.put(new String("two"), new Int(2));
    map.put(new String("three"), new Int(3));

    print("Testing HashMap key iterator:");

    Iterator<String> iter = map.iterator();
    int count = 0;

    while (iter.hasNext()) {
        String key = iter.next();
        print(key);
        count = count + 1;
    }

    iter.close();

    print("Count: " + count);
    print("HashMap key iterator test passed!");
}

main();
