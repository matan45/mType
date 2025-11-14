// Test enhanced for-loop with HashSet
import * from "../../lib/collections/HashSet.mt";
import * from "../../lib/primitives/Int.mt";


function main(): void {
    // Create a set
    HashSet<Int> set = new HashSet<Int>();
    set.add(5);
    set.add(10);
    set.add(15);
    set.add(20);

    // Test enhanced for-loop
    print("Testing enhanced for-loop with HashSet:");
    int sum = 0;
    for (Int numObj : set) {
        int num = numObj.getValue();
        print(num);
        sum = sum + num;
    }

    print("Sum: " + sum); // Should be 50

    print("Enhanced for-loop with HashSet test passed!");
}
main();
