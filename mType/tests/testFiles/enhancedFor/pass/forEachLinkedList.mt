// Test enhanced for-loop with LinkedList
import * from "../../lib/collections/LinkedList.mt";
import * from "../../lib/primitives/Int.mt";

@Script
function main(): void {
    // Create a linked list
    LinkedList<Int> list = new LinkedList<Int>();
    list.add(1);
    list.add(2);
    list.add(3);
    list.add(4);
    list.add(5);

    // Test enhanced for-loop
    print("Testing enhanced for-loop with LinkedList:");
    int product = 1;
    for (Int numObj : list) {
        int num = numObj.getValue();
        print(num);
        product = product * num;
    }

    print("Product: " + product); // Should be 120

    print("Enhanced for-loop with LinkedList test passed!");
}
