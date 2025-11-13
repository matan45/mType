// Test enhanced for-loop with LinkedList
import * from "../../lib/collections/LinkedList.mt";

@Script
function main(): void {
    // Create a linked list
    LinkedList<int> list = new LinkedList<int>();
    list.add(1);
    list.add(2);
    list.add(3);
    list.add(4);
    list.add(5);

    // Test enhanced for-loop
    print("Testing enhanced for-loop with LinkedList:");
    int product = 1;
    for (int num : list) {
        print(num);
        product = product * num;
    }

    print("Product: " + product); // Should be 120

    print("Enhanced for-loop with LinkedList test passed!");
}
