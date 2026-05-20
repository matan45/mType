// Error: HashSet iterator next() past end returns null; deref throws NPE
import * from "../../lib/collections/HashSet.mt";
import * from "../../lib/Iterator.mt";
import * from "../../lib/primitives/Int.mt";

HashSet<Int> s = new HashSet<Int>();
s.add(new Int(1));
s.add(new Int(2));

Iterator<Int> iter = s.iterator();
Int a = iter.next();
Int b = iter.next();
Int c = iter.next();
print("should not reach: " + c.getValue());
