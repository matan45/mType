// Test ArrayQueue as stream source - FIFO order preserved through stream
import * from "../../lib/collections/ArrayQueue.mt";
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    print("Testing Stream from ArrayQueue:");

    ArrayQueue<Int> queue = new ArrayQueue<Int>();
    queue.enqueue(new Int(10));
    queue.enqueue(new Int(20));
    queue.enqueue(new Int(30));

    // map: x -> x + 1, expect FIFO order: 11, 21, 31
    Int[] mapped = queue.stream()
        .map<Int>(v -> new Int(v.getValue() + 1))
        .toArray();

    print("Length: " + mapped.length);
    for (Int v : mapped) {
        print(v);
    }

    // count
    int total = queue.stream().count();
    print("Total: " + total);

    print("Stream ArrayQueue source test passed!");
}

main();
