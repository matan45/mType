// Test enhanced for-loop with empty collection
import * from "../../lib/collections/ArrayList.mt";

@Script
function main(): void {
    // Create an empty list
    ArrayList<int> list = new ArrayList<int>();

    // Test enhanced for-loop (should not execute body)
    print("Testing enhanced for-loop with empty collection:");
    int count = 0;
    for (int num : list) {
        count = count + 1;
    }

    if (count == 0) {
        print("Loop body correctly not executed for empty collection");
    }

    print("Empty collection for-loop test passed!");
}
