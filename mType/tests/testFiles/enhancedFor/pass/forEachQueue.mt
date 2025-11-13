// Test enhanced for-loop with Queue
import * from "../../lib/collections/ArrayQueue.mt";
import * from "../../lib/primitives/String.mt";


function main(): void {
    // Create a queue
    ArrayQueue<String> queue = new ArrayQueue<String>();
    queue.enqueue("First");
    queue.enqueue("Second");
    queue.enqueue("Third");

    // Test enhanced for-loop
    print("Testing enhanced for-loop with Queue:");
    for (String item : queue) {
        print(item);
    }

    print("Enhanced for-loop with Queue test passed!");
}
main();
