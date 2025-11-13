// Test iterator functionality on LinkedList
import * from "../../lib/collections/LinkedList.mt";
import * from "../../lib/Iterator.mt";

@Script
function main(): void {
    // Create a linked list with some elements
    LinkedList<int> list = new LinkedList<int>();
    list.add(100);
    list.add(200);
    list.add(300);

    // Get iterator
    Iterator<int> iter = list.iterator();

    // Test iteration
    print("Testing LinkedList iterator:");
    while (iter.hasNext()) {
        int value = iter.next();
        print(value);
    }

    // Close iterator
    iter.close();

    print("LinkedList iterator test passed!");
}
