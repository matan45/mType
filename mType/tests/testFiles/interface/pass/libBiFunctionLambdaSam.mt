// MYT-221 lock-in: lib/functional/BiFunction.mt is now a true SAM interface.
// `BiFunction<T, U, R>` declares only `apply`, so a lambda binds directly.

import * from "../../lib/functional/BiFunction.mt";
import * from "../../lib/primitives/Int.mt";

BiFunction<Int, Int, Int> add = (a, b) -> a.add(b);
print("add(10, 20) = " + add.apply(new Int(10), new Int(20)).getValue());
