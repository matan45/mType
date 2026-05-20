// Error: Stack iterator next() past end returns null; deref throws NPE
import * from "../../lib/collections/Stack.mt";
import * from "../../lib/Iterator.mt";
import * from "../../lib/primitives/Int.mt";

Stack<Int> s = new Stack<Int>();
s.push(new Int(1));
s.push(new Int(2));

Iterator<Int> iter = s.iterator();
Int a = iter.next();
Int b = iter.next();
Int c = iter.next();
print("should not reach: " + c.getValue());
