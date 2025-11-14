// Test: Stream reuse after intermediate operation followed by terminal operation
// Even intermediate operations consume the stream when chained with terminal operations

import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/primitives/Int.mt";

ArrayList<Int> list = new ArrayList<Int>();
list.add(new Int(10));
list.add(new Int(20));
list.add(new Int(30));

Stream<Int> stream = Streams::fromCollection<Int>(list);

// Create intermediate stream and consume it
Stream<Int> filtered = stream.filter(x -> new Bool(x.getValue() > 15));
int count = filtered.count();
print("Filtered count: " + count);

// Try to use the original stream - this should throw RuntimeException
// The original stream is consumed because filter() checks consumption
stream.forEach(x -> print(x.getValue()));
