// Test: Passing null mapper function to stream map should fail
// Stream map operation requires non-null mapper function

import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/functional/Function.mt";

ArrayList<Int> list = new ArrayList<Int>();
list.add(new Int(5));
list.add(new Int(10));

Stream<Int> stream = Streams::fromCollection<Int>(list);

// Pass null mapper - this should cause error when map tries to apply it
Function<Int, Int>? nullMapper = null;
Stream<Int> mapped = stream.map<Int>(nullMapper);

// Terminal operation triggers the error
mapped.forEach(x -> print(x.getValue()));
