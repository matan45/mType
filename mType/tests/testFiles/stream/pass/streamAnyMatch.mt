// Test stream anyMatch operation
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/primitives/Int.mt";



function main(): void {
    // Create a list
    ArrayList<Int> list = new ArrayList<Int>();
    list.add(1);
    list.add(5);
    list.add(15);
    list.add(3);

    // Test anyMatch
    print("Testing stream anyMatch:");
    bool hasLargeNumber = list.stream().anyMatch(value-> value > 10);

    if (hasLargeNumber) {
        print("Found at least one number > 10");
    }

    print("Stream anyMatch test passed!");
}
main();
