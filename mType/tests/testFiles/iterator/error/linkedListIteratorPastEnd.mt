// Test LinkedList iterator throws when next() is called past the end
import * from "../../lib/collections/LinkedList.mt";
import * from "../../lib/Iterator.mt";
import * from "../../lib/primitives/Int.mt";

LinkedList<Int> list = new LinkedList<Int>();
list.add(new Int(7));
list.add(new Int(8));

Iterator<Int> iter = list.iterator();
Int a = iter.next();
Int b = iter.next();
Int c = iter.next();
print("should not reach: " + c.getValue());
