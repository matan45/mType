// Test that calling next() immediately on an empty ArrayList iterator throws
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/Iterator.mt";
import * from "../../lib/primitives/Int.mt";

ArrayList<Int> list = new ArrayList<Int>();
Iterator<Int> iter = list.iterator();
Int v = iter.next();
print("should not reach: " + v.getValue());
