// Test: ArrayQueue iterator yields elements front-to-rear
import * from "../../lib/collections/ArrayQueue.mt";
import * from "../../lib/Iterator.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    ArrayQueue<Int> q = new ArrayQueue<Int>();
    q.enqueue(new Int(10));
    q.enqueue(new Int(20));
    q.enqueue(new Int(30));

    Iterator<Int> iter = q.iterator();
    while (iter.hasNext()) {
        Int v = iter.next();
        print(v.getValue());
    }
    iter.close();
    print("done");
}
main();

// Expected output:
// 10
// 20
// 30
// done
