// MYT-221 lock-in: lib/functional/Function.mt is now a true SAM interface.
// `Function<T, R>` declares only `apply`, so a lambda binds directly.

import * from "../../lib/functional/Function.mt";
import * from "../../lib/primitives/Int.mt";

Function<Int, Int> doubler = x -> new Int(x.getValue() * 2);
print("doubler(5) = " + doubler.apply(new Int(5)).getValue());
