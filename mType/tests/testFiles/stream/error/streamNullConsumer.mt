// Test: Passing null consumer to forEach should fail
// forEach terminal operation requires non-null consumer

import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/functional/Consumer.mt";

ArrayList<Int> list = new ArrayList<Int>();
list.add(new Int(100));
list.add(new Int(200));

Stream<Int> stream = Streams::fromCollection<Int>(list);

// Pass null consumer - this should cause error
Consumer<Int>? nullConsumer = null;
stream.forEach(nullConsumer);
