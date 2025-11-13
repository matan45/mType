// Test enhanced for-loop with HashSet
import * from "../../lib/collections/HashSet.mt";

@Script
function main(): void {
    // Create a set
    HashSet<int> set = new HashSet<int>();
    set.add(5);
    set.add(10);
    set.add(15);
    set.add(20);

    // Test enhanced for-loop
    print("Testing enhanced for-loop with HashSet:");
    int sum = 0;
    for (int num : set) {
        print(num);
        sum = sum + num;
    }

    print("Sum: " + sum); // Should be 50

    print("Enhanced for-loop with HashSet test passed!");
}
