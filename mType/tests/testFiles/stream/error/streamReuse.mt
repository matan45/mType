// Test: Stream reuse after terminal operation should fail
// Streams can only be used once - after a terminal operation, they are consumed

import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/primitives/Int.mt";

ArrayList<Int> list = new ArrayList<Int>();
list.add(new Int(1));
list.add(new Int(2));
list.add(new Int(3));

Stream<Int> stream = Streams::fromCollection<Int>(list);

// First terminal operation - this should work
int count = stream.count();
print("Count: " + count);

// Second terminal operation on same stream - this should throw RuntimeException
// Expected error: "Stream has already been operated upon or closed"
stream.forEach(x -> print(x.getValue()));
