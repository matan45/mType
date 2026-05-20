// Test: ArrayQueue iterator yields correct order after internal wrap-around
// Capacity is 4; enqueue 3, dequeue 1, enqueue 1, dequeue 1, enqueue 1 produces
// front=2, rear=1 (rear < front), so iteration must walk the circular buffer.
import * from "../../lib/collections/ArrayQueue.mt";
import * from "../../lib/Iterator.mt";
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

    Iterator<Int> iter = q.iterator();
    while (iter.hasNext()) {
        Int v = iter.next();
        print(v.getValue());
    }
    iter.close();
}
main();

// Expected output:
// 3
// 4
// 5
