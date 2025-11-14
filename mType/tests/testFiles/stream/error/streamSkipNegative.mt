// Test: Stream skip with negative value should fail
// skip() should only accept non-negative integers

import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/primitives/Int.mt";

ArrayList<Int> list = new ArrayList<Int>();
list.add(new Int(10));
list.add(new Int(20));
list.add(new Int(30));

Stream<Int> stream = Streams::fromCollection<Int>(list);

// Try to skip with negative value - this should fail
Stream<Int> skipped = stream.skip(-3);
skipped.forEach(x -> print(x.getValue()));
