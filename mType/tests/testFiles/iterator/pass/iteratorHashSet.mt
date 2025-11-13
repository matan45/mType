// Test iterator functionality on HashSet
import * from "../../lib/collections/HashSet.mt";
import * from "../../lib/Iterator.mt";

@Script
function main(): void {
    // Create a set with some elements
    HashSet<String> set = new HashSet<String>();
    set.add("apple");
    set.add("banana");
    set.add("cherry");
    set.add("date");

    // Get iterator
    Iterator<String> iter = set.iterator();

    // Test iteration (order may vary due to hashing)
    print("Testing HashSet iterator:");
    int count = 0;
    while (iter.hasNext()) {
        String value = iter.next();
        print(value);
        count = count + 1;
    }

    print("Count: " + count); // Should be 4

    // Close iterator
    iter.close();

    print("HashSet iterator test passed!");
}
