// Test enhanced for-loop with empty collection
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    // Create an empty list
    ArrayList<Int> list = new ArrayList<Int>();

    // Test enhanced for-loop (should not execute body)
    print("Testing enhanced for-loop with empty collection:");
    int count = 0;
    for (Int numObj : list) {
        count = count + 1;
    }

    if (count == 0) {
        print("Loop body correctly not executed for empty collection");
    }

    print("Empty collection for-loop test passed!");
}
main();
