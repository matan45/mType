// Test: Passing null predicate to stream filter should fail
// Stream operations require non-null functional arguments

import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/functional/Predicate.mt";

ArrayList<Int> list = new ArrayList<Int>();
list.add(new Int(1));
list.add(new Int(2));

Stream<Int> stream = Streams::fromCollection<Int>(list);

// Pass null predicate - this should cause a runtime error when filter tries to use it
Predicate<Int>? nullPredicate = null;
Stream<Int> filtered = stream.filter(nullPredicate);

// Terminal operation triggers the error
filtered.forEach(x -> print(x.getValue()));
