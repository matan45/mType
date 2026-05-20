// Test: ArrayQueue.contains() works after the internal buffer wraps
// Same sequence as iteratorArrayQueueAfterWrapAround.mt — produces front=2, rear=1.
import * from "../../lib/collections/ArrayQueue.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    ArrayQueue<Int> q = new ArrayQueue<Int>();
    q.enqueue(new Int(1));
    q.enqueue(new Int(2));
    q.enqueue(new Int(3));
    q.dequeue();
    q.enqueue(new Int(4));
    q.dequeue();
    q.enqueue(new Int(5));

    print("contains 3: " + q.contains(new Int(3)));
    print("contains 4: " + q.contains(new Int(4)));
    print("contains 5: " + q.contains(new Int(5)));
    print("contains 1 (dequeued): " + q.contains(new Int(1)));
    print("contains 99: " + q.contains(new Int(99)));
}
main();

// Expected output:
// contains 3: true
// contains 4: true
// contains 5: true
// contains 1 (dequeued): false
// contains 99: false
