// Test: Stream.sorted() always throws (current impl per StreamImpl.mt:77).
// "Stream.sorted() requires elements to implement Comparable interface.
//  Use sortedWith(Comparator) instead."
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/primitives/Int.mt";

ArrayList<Int> list = new ArrayList<Int>();
list.add(new Int(3));
list.add(new Int(1));
list.add(new Int(2));

// sorted() unconditionally throws today
Stream<Int> sorted = list.stream().sorted();
sorted.toArray();
