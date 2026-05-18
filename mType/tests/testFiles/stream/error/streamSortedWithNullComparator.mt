// Test: Passing null comparator to sortedWith should fail.
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/functional/Comparator.mt";
import * from "../../lib/primitives/Int.mt";

ArrayList<Int> list = new ArrayList<Int>();
list.add(new Int(3));
list.add(new Int(1));
list.add(new Int(2));

Comparator<Int>? nullComp = null;
Stream<Int> sorted = list.stream().sortedWith(nullComp);
sorted.toArray();
