// Test ArrayQueue resize (triggered when count >= capacity) preserves FIFO order
import * from "../../lib/collections/ArrayQueue.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    print("Testing ArrayQueue resize preserves order:");

    ArrayQueue<Int> q = new ArrayQueue<Int>();
    for (int i = 1; i <= 9; i++) {
        q.enqueue(new Int(i));
    }
    print("size: " + q.size());

    while (!q.empty()) {
        Int v = q.dequeue();
        print("out: " + v.getValue());
    }

    print("ArrayQueue resize order test passed!");
}
main();
