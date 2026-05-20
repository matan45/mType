// Error: ArrayQueue iterator next() past end returns null; deref throws NPE
import * from "../../lib/collections/ArrayQueue.mt";
import * from "../../lib/Iterator.mt";
import * from "../../lib/primitives/Int.mt";

ArrayQueue<Int> q = new ArrayQueue<Int>();
q.enqueue(new Int(10));
q.enqueue(new Int(20));

Iterator<Int> iter = q.iterator();
Int a = iter.next();
Int b = iter.next();
Int c = iter.next();
print("should not reach: " + c.getValue());
