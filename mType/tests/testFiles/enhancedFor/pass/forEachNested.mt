// Test nested enhanced for-loops
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";


function main(): void {
    // Create lists
    ArrayList<Int> list1 = new ArrayList<Int>();
    list1.add(1);
    list1.add(2);

    ArrayList<Int> list2 = new ArrayList<Int>();
    list2.add(10);
    list2.add(20);

    // Test nested for-loops
    print("Testing nested enhanced for-loops:");
    for (Int xObj : list1) {
        int x = xObj.getValue();
        for (Int yObj : list2) {
            int y = yObj.getValue();
            int result = x * y;
            print("" + x + " * " + y + " = " + result);
        }
    }

    print("Nested for-loop test passed!");
}

main();
