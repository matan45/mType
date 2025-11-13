// Test iterator functionality on LinkedList
import * from "../../lib/collections/LinkedList.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/Iterator.mt";


function main(): void {
    // Create a linked list with some elements
    LinkedList<Int> list = new LinkedList<Int>();
    list.add(100);
    list.add(200);
    list.add(300);

    // Get iterator
    Iterator<Int> iter = list.iterator();

    // Test iteration
    print("Testing LinkedList iterator:");
    while (iter.hasNext()) {
        Int valueObj = iter.next();
        int value = valueObj.getValue();
        print(value);
    }

    // Close iterator
    iter.close();

    print("LinkedList iterator test passed!");
}
main();
