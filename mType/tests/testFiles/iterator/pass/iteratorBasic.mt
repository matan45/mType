// Test basic iterator functionality on ArrayList
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/Iterator.mt";


function main(): void {
    // Create a list with some elements
    ArrayList<Int> list = new ArrayList<Int>();
    list.add(10);
    list.add(20);
    list.add(30);
    list.add(40);

    // Get iterator
    Iterator<Int> iter = list.iterator();

    // Test hasNext and next
    print("Testing ArrayList iterator:");
    int sum = 0;
    while (iter.hasNext()) {
        Int valueObj = iter.next();
        int value = valueObj.getValue();
        print(value);
        sum = sum + value;
    }

    print("Sum: " + sum); // Should be 100

    // Close iterator
    iter.close();

    print("Iterator test passed!");
}

main();
