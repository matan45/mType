// Test: ArrayQueue.dequeue() and ArrayQueue.peek() on an empty queue return null
import * from "../../lib/collections/ArrayQueue.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    ArrayQueue<Int> q = new ArrayQueue<Int>();
    print("empty: " + q.empty());
    print("size: " + q.size());

    Int d = q.dequeue();
    print("dequeue empty null: " + (d == null));

    Int pk = q.peek();
    print("peek empty null: " + (pk == null));

    q.enqueue(new Int(5));
    Int d2 = q.dequeue();
    print("dequeue after enqueue: " + d2.getValue());

    Int d3 = q.dequeue();
    print("dequeue empty again null: " + (d3 == null));
}
main();

// Expected output:
// empty: true
// size: 0
// dequeue empty null: true
// peek empty null: true
// dequeue after enqueue: 5
// dequeue empty again null: true
