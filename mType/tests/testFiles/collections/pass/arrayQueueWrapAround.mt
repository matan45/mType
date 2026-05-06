// Test ArrayQueue circular buffer wrap-around with mixed enqueue/dequeue
// Initial capacity = 4. Sequence forces front and rear past buffer end without resize.
import * from "../../lib/collections/ArrayQueue.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    print("Testing ArrayQueue wrap-around:");

    ArrayQueue<Int> q = new ArrayQueue<Int>();
    q.enqueue(new Int(1));
    q.enqueue(new Int(2));
    q.enqueue(new Int(3));

    Int a = q.dequeue();
    print("dq1: " + a.getValue());
    Int b = q.dequeue();
    print("dq2: " + b.getValue());

    q.enqueue(new Int(4));
    q.enqueue(new Int(5));
    q.enqueue(new Int(6));

    print("size: " + q.size());

    while (!q.empty()) {
        Int v = q.dequeue();
        print("out: " + v.getValue());
    }

    print("ArrayQueue wrap-around test passed!");
}
main();
