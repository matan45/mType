// Error: ArrayQueue.dequeue() on empty returns null; dereferencing throws NPE
import * from "../../lib/collections/ArrayQueue.mt";
import * from "../../lib/primitives/Int.mt";

ArrayQueue<Int> q = new ArrayQueue<Int>();
Int v = q.dequeue();
int x = v.getValue();
print("should not reach: " + x);
