// Test: Passing null predicate to allMatch should fail.
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/functional/Predicate.mt";

ArrayList<Int> list = new ArrayList<Int>();
list.add(new Int(1));
list.add(new Int(2));

Predicate<Int>? nullPredicate = null;
list.stream().allMatch(nullPredicate);
