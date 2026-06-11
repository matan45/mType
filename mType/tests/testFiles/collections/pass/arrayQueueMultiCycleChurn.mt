// Test: 20 enqueue/dequeue cycles lap the circular buffer many times
// (existing wrap-around test covers a single lap). FIFO order must hold on
// every cycle; the checksum is order-sensitive.
import * from "../../lib/collections/ArrayQueue.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    print("Testing ArrayQueue multi-cycle churn:");

    ArrayQueue<Int> q = new ArrayQueue<Int>();
    int checksum = 0;
    for (int cycle = 0; cycle < 20; cycle++) {
        q.enqueue(new Int(cycle * 3 + 1));
        q.enqueue(new Int(cycle * 3 + 2));
        q.enqueue(new Int(cycle * 3 + 3));
        Int a = q.dequeue();
        Int b = q.dequeue();
        Int c = q.dequeue();
        checksum = checksum * 31 + a.getValue();
        checksum = checksum * 31 + b.getValue();
        checksum = checksum * 31 + c.getValue();
    }
    print("empty after churn: " + q.empty());
    print("checksum: " + checksum);

    print("ArrayQueue multi-cycle churn test passed!");
}
main();
