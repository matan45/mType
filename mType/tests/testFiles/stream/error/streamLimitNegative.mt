// Test: Stream limit with negative value should fail
// limit() should only accept non-negative integers

import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/primitives/Int.mt";

ArrayList<Int> list = new ArrayList<Int>();
list.add(new Int(1));
list.add(new Int(2));
list.add(new Int(3));

Stream<Int> stream = Streams::fromCollection<Int>(list);

// Try to limit with negative value - this should fail
Stream<Int> limited = stream.limit(-5);
limited.forEach(x -> print(x.getValue()));
