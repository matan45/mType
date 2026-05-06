// Test ArrayList iterator throws when next() is called past the end
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/Iterator.mt";
import * from "../../lib/primitives/Int.mt";

ArrayList<Int> list = new ArrayList<Int>();
list.add(new Int(10));
list.add(new Int(20));

Iterator<Int> iter = list.iterator();
Int a = iter.next();
Int b = iter.next();
Int c = iter.next();
print("should not reach: " + c.getValue());
