// Test: HashMap value iterator
// Tests valueIterator() method on HashMap

import * from "../../lib/collections/HashMap.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/Iterator.mt";

function main(): void {
    HashMap<String, Int> map = new HashMap<String, Int>();
    map.put(new String("x"), new Int(10));
    map.put(new String("y"), new Int(20));
    map.put(new String("z"), new Int(30));

    print("Testing HashMap value iterator:");

    Iterator<Int> iter = map.valueIterator();
    int sum = 0;
    int count = 0;

    while (iter.hasNext()) {
        Int value = iter.next();
        print(value);
        sum = sum + value.value;
        count = count + 1;
    }

    iter.close();

    print("Count: " + count);
    print("Sum: " + sum);
    print("HashMap value iterator test passed!");
}

main();
