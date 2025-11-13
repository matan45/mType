// Test nested enhanced for-loops
import * from "../../lib/collections/ArrayList.mt";

@Script
function main(): void {
    // Create lists
    ArrayList<int> list1 = new ArrayList<int>();
    list1.add(1);
    list1.add(2);

    ArrayList<int> list2 = new ArrayList<int>();
    list2.add(10);
    list2.add(20);

    // Test nested for-loops
    print("Testing nested enhanced for-loops:");
    for (int x : list1) {
        for (int y : list2) {
            int result = x * y;
            print("" + x + " * " + y + " = " + result);
        }
    }

    print("Nested for-loop test passed!");
}
